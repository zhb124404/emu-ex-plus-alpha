/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define thisModuleName "res:face"

#include <resource2/image/glyph/ResourceImageGlyph.h>
#include <util/strings.h>
#include "ResourceFace.hh"

#ifdef CONFIG_RESOURCE_FONT_FREETYPE
	#include <resource2/font/ResourceFontFreetype.hh>
	#ifdef CONFIG_PACKAGE_FONTCONFIG
		#include <fontconfig/fontconfig.h>
	#endif
#elif defined CONFIG_RESOURCE_FONT_ANDROID
	#include <resource2/font/ResourceFontAndroid.hh>
#elif defined CONFIG_RESOURCE_FONT_UIKIT
	#include <resource2/font/ResourceFontUIKit.hh>
#endif

// definitions for the Unicode Basic Multilingual Plane (BMP)
static const uint unicodeBmpChars = 0xFFFE;

// location & size of the surrogate/private chars
static const uint unicodeBmpPrivateStart = 0xD800, unicodeBmpPrivateEnd = 0xF8FF;
static const uint unicodeBmpPrivateChars = 0x2100;

static const uint unicodeBmpUsedChars = unicodeBmpChars - unicodeBmpPrivateChars;

static const uint glyphTableEntries = ResourceFace::supportsUnicode ? unicodeBmpUsedChars : numDrawableAsciiChars;

void ResourceFace::initGlyphTable()
{
	//logMsg("initGlyphTable");
	iterateTimes(glyphTableEntries, i)
	{
		glyphTable[i].glyph = nullptr;
	}
}

ResourceFace *ResourceFace::load(const char *path, FontSettings *set)
{
	ResourceFont *font = nullptr;
	#ifdef CONFIG_RESOURCE_FONT_FREETYPE
		font = ResourceFontFreetype::load(path);
	#endif
	if(!font)
		return nullptr;

	return create(font, set);
}

ResourceFace *ResourceFace::load(Io* io, FontSettings *set)
{
	ResourceFont *font = nullptr;
	#ifdef CONFIG_RESOURCE_FONT_FREETYPE
		font = ResourceFontFreetype::load(io);
	#endif
	if(!font)
		return nullptr;

	return create(font, set);
}

#ifdef CONFIG_PACKAGE_FONTCONFIG
static CallResult getFontFilenameWithPattern(FcPattern *pat, char *&filename, FcPattern *&matchPatOut)
{
	FcDefaultSubstitute(pat);
	if(!FcConfigSubstitute(nullptr, pat, FcMatchPattern))
	{
		logErr("error applying font substitutions");
		FcPatternDestroy(pat);
		return INVALID_PARAMETER;
	}
	FcResult result;
	auto matchPat = FcFontMatch(nullptr, pat, &result);
	FcPatternDestroy(pat);
	if(!matchPat || result == FcResultNoMatch)
	{
		logErr("fontconfig couldn't find a valid font");
		if(matchPat)
			FcPatternDestroy(matchPat);
		return INVALID_PARAMETER;
	}
	if(!FcPatternGetString(matchPat, FC_FILE, 0, (FcChar8**)&filename) == FcResultMatch)
	{
		logErr("fontconfig font missing file path");
		FcPatternDestroy(matchPat);
		return INVALID_PARAMETER;
	}
	matchPatOut = matchPat;
	return OK;
}

static ResourceFontFreetype *loadSystemSans()
{
	auto pat = FcPatternCreate();
	if(!pat)
	{
		logErr("error allocating fontconfig pattern");
		return nullptr;
	}
	FcPatternAddString(pat, FC_FAMILY, (const FcChar8*)"sans");
	char *filename;
	FcPattern *matchPat;
	if(getFontFilenameWithPattern(pat, filename, matchPat) != OK)
	{
		return nullptr;
	}
	auto font = ResourceFontFreetype::load(filename);
	FcPatternDestroy(matchPat);
	return font;
}

static void addonSystemJa(ResourceFontFreetype *font)
{
	auto pat = FcPatternCreate();
	if(!pat)
	{
		logErr("error allocating fontconfig pattern");
		return;
	}
	FcPatternAddString(pat, FC_LANG, (const FcChar8*)"ja");
	char *filename;
	FcPattern *matchPat;
	if(getFontFilenameWithPattern(pat, filename, matchPat) != OK)
	{
		return;
	}
	font->loadIntoSlot((char*)filename, 1);
	FcPatternDestroy(matchPat);
}
#endif

ResourceFace *ResourceFace::loadSystem(FontSettings *set)
{
	#ifdef CONFIG_RESOURCE_FONT_ANDROID
		auto *font = ResourceFontAndroid::loadSystem();
		if(!font)
			return nullptr;
		return create(font, set);
	#elif defined CONFIG_RESOURCE_FONT_UIKIT
		auto *font = ResourceFontUIKit::loadSystem();
		if(!font)
			return nullptr;
		return create(font, set);
	#elif defined CONFIG_ENV_WEBOS
		return load("/usr/share/fonts/PreludeCondensed-Medium.ttf", set);
	#else
		#ifdef CONFIG_PACKAGE_FONTCONFIG
			logMsg("locating system fonts with fontconfig");
			if(!FcInitLoadConfigAndFonts())
			{
				logErr("error initializing fontconfig");
				return nullptr;
			}
			auto font = loadSystemSans();
			if(!font)
				return nullptr;
			addonSystemJa(font);
			return create(font, set);
		#else
			return loadAsset("Vera.ttf", set);
		#endif
	#endif
}

ResourceFace *ResourceFace::create(ResourceFace *face, FontSettings *set)
{
	return create(face->font, set);
}

ResourceFace *ResourceFace::create(ResourceFont *font, FontSettings *set)
{
	ResourceFace *inst = new ResourceFace;
	if(!inst)
	{
		logErr("out of memory");
		return nullptr;
	}

	logMsg("allocating glyph table, %d entries", glyphTableEntries);
	inst->glyphTable = (GlyphEntry*)mem_alloc(sizeof(GlyphEntry) * glyphTableEntries);
	if(!inst->glyphTable)
	{
		delete inst;
		logErr("out of memory");
		return nullptr;
	}

	inst->font = font;
	inst->initGlyphTable();

	if(set)
	{
		inst->settings = *set;
		inst->settings.process();
		font->newSize(&inst->settings, inst->faceSize);
	}
	else
	{
		inst->settings.pixelHeight = 0;
		inst->settings.pixelWidth = 0;;
	}

	return inst;
}

void ResourceFace::free ()
{
	font->freeSize(faceSize);
	iterateTimes(glyphTableEntries, i)
	{
		glyphTable[i].glyph->freeSafe();
	}
	mem_free(glyphTable);
	delete this;
}

uint ResourceFace::nominalHeight() const
{
	return nominalHeight_;
}

void ResourceFace::calcNominalHeight()
{
	//logMsg("calcNominalHeight");
	GlyphEntry *mGly = glyphEntry('M');
	GlyphEntry *gGly = glyphEntry('g');

	assert(mGly != NULL && gGly != NULL);

	nominalHeight_ = mGly->metrics.ySize + (gGly->metrics.ySize/2);
}

CallResult ResourceFace::applySettings (FontSettings set)
{
	set.process();
	if(set.pixelWidth < font->minUsablePixels())
		set.pixelWidth = font->minUsablePixels();
	if(set.pixelHeight < font->minUsablePixels())
		set.pixelHeight = font->minUsablePixels();

	if(set != settings)
	{
		if(settings.areValid())
		{
			logMsg("flushing glyph cache");
			font->freeSize(faceSize);
			iterateTimes(glyphTableEntries, i)
			{
				glyphTable[i].glyph->freeSafe();
			}
		}

		settings = set;
		font->newSize(&settings, faceSize);
		initGlyphTable();
		calcNominalHeight();
		return OK;
	}
	else
		return RESOURCE_FACE_SETTINGS_UNCHANGED;
}

//int ResourceFace::maxDescender () { font->applySize(faceSize); return font->currentFaceDescender(); }
//int ResourceFace::maxAscender () { font->applySize(faceSize); return font->currentFaceAscender(); }

CallResult ResourceFace::writeCurrentChar(Pixmap *out)
{
	void *bitmap = nullptr;
	int bX = 0, bY = 0, bPitch;
	font->charBitmap(bitmap, bX, bY, bPitch);
	//logDMsg("copying char %dx%d, pitch %d to dest %dx%d, pitch %d", bX, bY, bPitch, out->x, out->y, out->pitch);
	assert(bX != 0 && bY != 0 && bitmap != nullptr);
	Pixmap src(PixelFormatI8);
	src.init((uchar*)bitmap, bX, bY, bPitch - bX);
	src.copy(0, 0, 0, 0, out, 0, 0);
	//memset ( out->data, 0xFF, 16 ); // test by filling with white
	font->unlockCharBitmap(bitmap);
	return OK;
}

CallResult ResourceFace::cacheChar(int c, int tableIdx)
{
	GlyphMetrics metrics;
	// make sure applySize() has been called on the font object first
	doOrReturn(font->activeChar(c, metrics));
	//logMsg("setting up table entry %d", tableIdx);
	glyphTable[tableIdx].metrics = metrics;
	glyphTable[tableIdx].glyph = font->createRenderable(c, this, &glyphTable[tableIdx]);
	return OK;
}

static CallResult mapCharToTable(uint c, uint &tableIdx)
{
	if(ResourceFace::supportsUnicode)
	{
		if(c < unicodeBmpChars && charIsDrawableUnicode(c))
		{
			if(c < unicodeBmpPrivateStart)
			{
				tableIdx = c;
				return OK;
			}
			else if(c > unicodeBmpPrivateEnd)
			{
				tableIdx = c - unicodeBmpPrivateChars; // surrogate & private chars are a hole in the table
				return OK;
			}
			else
				return INVALID_PARAMETER;
		}
		else return INVALID_PARAMETER;
	}
	else
	{
		if(charIsDrawableAscii(c))
		{
			tableIdx = c - firstDrawableAsciiChar;
			return OK;
		}
		else return INVALID_PARAMETER;
	}
}

CallResult ResourceFace::precache(const char *string)
{
	assert(settings.areValid());
	font->applySize(faceSize);
	for(int i = 0, c = string[i]; c != 0; c = string[++i])
	{
		uint tableIdx;
		if(mapCharToTable(c, tableIdx) != OK)
		{
			//logMsg( "%c not a known drawable character, skipping", c);
			continue;
		}
		if(glyphTable[tableIdx].glyph)
		{
			//logMsg( "%c already cached", c);
			continue;
		}

		logMsg( "precaching char %c", c);
		doOrReturn(cacheChar(c, tableIdx));
	}
	return OK;
}

GlyphEntry *ResourceFace::glyphEntry(int c)
{
	assert(settings.areValid());
	uint tableIdx;
	if(mapCharToTable(c, tableIdx) != OK)
		return nullptr;
	assert(tableIdx < glyphTableEntries);
	if(!glyphTable[tableIdx].glyph)
	{
		//logMsg("char %c not in table, caching now", c);
		font->applySize(faceSize);
		if(cacheChar(c, tableIdx))
			return nullptr;
	}

	return &glyphTable[tableIdx];
}
