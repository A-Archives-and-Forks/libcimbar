/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimb_translator/Config.h"
#include "compression/zstd_decompressor.h"
#include "encoder/DecoderPlus.h"
#include "encoder/EncoderPlus.h"
#include "extractor/Extractor.h"
#include "extractor/SimpleCameraCalibration.h"
#include "extractor/Undistort.h"
#include "fountain/FountainInit.h"
#include "fountain/fountain_decoder_sink.h"
#include "serialize/str.h"

#include "cxxopts/cxxopts.hpp"

#include <cstdio>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace {
	class stdinerator
	{
	public:
		stdinerator(bool done=false)
			: _done(done)
		{
			read_once();
		}

		void mark_done()
		{
			_done = true;
		}

		void read_once()
		{
			if (_done)
				return;
#ifdef _WIN32
			if (::feof(stdin))
#else
			if (::feof(::stdin))
#endif
			{
				mark_done();
				return;
			}

			std::getline(std::cin, _current);
		}

		std::string operator*() const
		{
			return _current;
		}

		stdinerator& operator++()
		{
			read_once();
			return *this;
		}

		bool operator==(const stdinerator& rhs) const
		{
			return _done and rhs._done;
		}

		bool operator!=(const stdinerator& rhs) const
		{
			return !operator==(rhs);
		}

		static stdinerator begin()
		{
			return stdinerator();
		}

		static stdinerator end()
		{
			return stdinerator(true);
		}

	protected:
		bool _done = false;
		std::string _current;
	};

	struct StdinLineReader
	{
		stdinerator begin() const
		{
			return stdinerator::begin();
		}

		stdinerator end() const
		{
			return stdinerator::end();
		}
	};
}

template <typename FilenameIterable>
int encode(const FilenameIterable& infiles, const std::string& outpath, int compression_level, bool no_fountain)
{
	EncoderPlus en;
	en.set_encode_id(109);
	for (const string& f : infiles)
	{
		if (f.empty())
			continue;
		if (no_fountain)
			en.encode(f, outpath);
		else
			en.encode_fountain(f, outpath, compression_level);
	}
	return 0;
}

template <typename FilenameIterable>
int decode(const FilenameIterable& infiles, const std::function<int(cv::UMat, bool, int)>& decodefun, bool no_deskew, bool undistort, int preprocess, int color_correct)
{
	int err = 0;
	for (const string& inf : infiles)
	{
		if (inf.empty())
			continue;
		bool shouldPreprocess = (preprocess == 1);
		cv::UMat img = cv::imread(inf).getUMat(cv::ACCESS_RW);
		cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

		if (!no_deskew)
		{
			// attempt undistort. It's currently a low-effort attempt to *reduce* distortion, not eliminate it.
			// we rely on the decoder to power through minor distortion
			if (undistort)
			{
				Undistort<SimpleCameraCalibration> und;
				if (!und.undistort(img, img))
					err |= 1;
			}

			Extractor ext;
			int res = ext.extract(img, img);
			if (!res)
			{
				err |= 2;
				continue;
			}
			else if (preprocess != 0 and res == Extractor::NEEDS_SHARPEN)
				shouldPreprocess = true;
		}

		int bytes = decodefun(img, shouldPreprocess, color_correct);
		if (!bytes)
			err |= 4;
	}
	return err;
}

// see also "decodefun" for non-fountain decodes, defined as a lambda inline below.
// this one needs its own function since it's a template (:
template <typename SINK>
std::function<int(cv::UMat,bool,int)> fountain_decode_fun(SINK& sink, Decoder& d)
{
	return [&sink, &d] (cv::UMat m, bool pre, int cc) {
		return d.decode_fountain(m, sink, pre, cc);
	};
}

int main(int argc, char** argv)
{
	cxxopts::Options options("cimbar encoder/decoder", "Demonstration program for cimbar codes");

	unsigned compressionLevel = cimbar::Config::compression_level();
	options.add_options()
		("n,encode", "Run the encoder!", cxxopts::value<bool>())
		("i,in", "Encoded pngs/jpgs/etc (for decode), or file to encode", cxxopts::value<vector<string>>())
		("o,out", "Output file prefix (encoding) or directory (decoding).", cxxopts::value<string>())
		("m,mode", "Select a cimbar mode. B (the default) is new to 0.6.x. 4C is the 0.5.x config. [B,Bm,4C]", cxxopts::value<string>()->default_value("B"))
		("z,compression", "Compression level. 0 == no compression.", cxxopts::value<int>()->default_value(turbo::str::str(compressionLevel)))
		("color-correct", "Toggle decoding color correction. 2 == full (fountain mode only). 1 == simple. 0 == off.", cxxopts::value<int>()->default_value("2"))
		("color-correction-file", "Debug -- save color correction matrix generated during fountain decode, or use it for non-fountain decodes", cxxopts::value<string>())
		("no-deskew", "Skip the deskew step -- treat input image as already extracted.", cxxopts::value<bool>())
		("no-fountain", "Disable fountain encode/decode. Will also disable compression.", cxxopts::value<bool>())
		("undistort", "Attempt undistort step -- useful if image distortion is significant.", cxxopts::value<bool>())
		("preprocess", "Run sharpen filter on the input image. 1 == on. 0 == off. -1 == guess.", cxxopts::value<int>()->default_value("-1"))
		("h,help", "Print usage")
	;
	options.show_positional_help();
	options.parse_positional({"in"});
	options.positional_help("<in...>");

	auto result = options.parse(argc, argv);
	if (result.count("help"))
	{
		std::cerr << options.help() << std::endl;
		return 0;
	}

	string outpath = std::filesystem::current_path().string();
	if (result.count("out"))
		outpath = result["out"].as<string>();
	std::cerr << "Output files will appear in " << outpath << std::endl;

	bool useStdin = !result.count("in");
	vector<string> infiles;
	if (!useStdin)
	{
		infiles = result["in"].as<vector<string>>();
		if (infiles.empty())
		{
			std::cerr << "No input files? :(" << std::endl;
			return 128;
		}
	}
	else
		std::cerr << "Enter input filenames:" << std::endl;

	bool encodeFlag = result.count("encode");
	bool no_fountain = result.count("no-fountain");

	compressionLevel = result["compression"].as<int>();

	// set config
	unsigned config_mode = 68;
	if (result.count("mode"))
	{
		string mode = result["mode"].as<string>();
		if (mode == "4" or mode == "4c" or mode == "4C")
			config_mode = 4;
		else if (mode == "8c" or mode == "8C")
			config_mode = 8;
		else if (mode == "Bm" or mode == "BM")
			config_mode = 67;
	}
	cimbar::Config::update(config_mode);

	if (encodeFlag)
	{
		if (useStdin)
			return encode(StdinLineReader(), outpath, compressionLevel, no_fountain);
		else
			return encode(infiles, outpath, compressionLevel, no_fountain);
	}

	// else, decode
	bool no_deskew = result.count("no-deskew");
	bool undistort = result.count("undistort");
	int color_correct = result["color-correct"].as<int>();
	string color_correction_file;
	if (result.count("color-correction-file"))
		color_correction_file = result["color-correction-file"].as<string>();
	int preprocess = result["preprocess"].as<int>();

	DecoderPlus d;

	if (no_fountain)
	{
		if (not color_correction_file.empty())
			d.load_ccm(color_correction_file);

		// simpler encoding, just the basics + ECC. No compression, fountain codes, etc.
		std::ofstream f(outpath);
		std::function<int(cv::UMat,bool,int)> decodefun = [&f, &d] (cv::UMat m, bool pre, int cc) {
			return d.decode(m, f, pre, cc);
		};
		if (useStdin)
			return decode(StdinLineReader(), decodefun, no_deskew, undistort, preprocess, color_correct);
		else
			return decode(infiles, decodefun, no_deskew, undistort, preprocess, color_correct);
	}

	// else, the good stuff
	int res = -200;

	unsigned chunkSize = cimbar::Config::fountain_chunk_size();
	if (compressionLevel <= 0)
	{
		fountain_decoder_sink sink(chunkSize, write_on_store<std::ofstream>(outpath, true));
		res = decode(infiles, fountain_decode_fun(sink, d), no_deskew, undistort, preprocess, color_correct);
	}
	else // default case, all bells and whistles
	{
		fountain_decoder_sink sink(chunkSize, write_on_store<cimbar::zstd_decompressor<std::ofstream>>(outpath, true));

		if (useStdin)
			res = decode(StdinLineReader(), fountain_decode_fun(sink, d), no_deskew, undistort, preprocess, color_correct);
		else
			res = decode(infiles, fountain_decode_fun(sink, d), no_deskew, undistort, preprocess, color_correct);
	}
	if (not color_correction_file.empty())
		d.save_ccm(color_correction_file);

	return res;
}
