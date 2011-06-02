

if (_ACTION == 'gmake') then
  project 'libftgl'
    kind 'StaticLib'
    language 'C'
    hidetarget('true')
    configuration 'vs*'
      foreignproject 'ftgl/msvc/vc8/ftgl_static.vcproj'
      foreigntarget  'ftgl/build/ftgl_static.lib'
      uuid           '1D758EEA-59C3-46E4-BEF5-16DCCA8C0B21'
    configuration 'not vs*'
      foreignproject 'ftgl'
      foreigntarget  'src/.libs/libftgl.a'
      foreignconfig {
        'if [ ! -f ./configure ]; then ./autogen.sh; fi;',
        'if [ ! -f ./Makefile ];  then ./configure --enable-shared=no;  fi;',
      }
      foreignbuild      'cd src ; $(MAKE)'
      foreignclean      '$(MAKE) clean'
      foreignsuperclean '$(MAKE) distclean'

      targetname   'ftgl'            -- FIXME - should not be required
      targetdir    'ftgl/src/.libs'
  return
end



project   'libftgl'
  targetname 'ftgl'
  hidetarget('true')
  kind 'StaticLib'
  language 'C++'
  objdir '.objs_ftgl'
  includedirs {
    'ftgl',
    'ftgl/src/',
    'ftgl/src/FTGL',
    'ftgl/src/FTFont',
    'ftgl/src/FTGlyph',
    'ftgl/src/FTLayout',
  }
  includepackage 'freetype'
  files {
    'ftgl/src/FTBuffer.cpp',
    'ftgl/src/FTCharmap.cpp',
    'ftgl/src/FTCleanup.cpp',
    'ftgl/src/FTContour.cpp',
    'ftgl/src/FTFace.cpp',
    'ftgl/src/FTGL.cpp',
    'ftgl/src/FTGlyphContainer.cpp',
    'ftgl/src/FTLibrary.cpp',
    'ftgl/src/FTPoint.cpp',
    'ftgl/src/FTSize.cpp',
    'ftgl/src/FTVectoriser.cpp',
    'ftgl/src/FTFont/FTBitmapFont.cpp',
    'ftgl/src/FTFont/FTBufferFont.cpp',
    'ftgl/src/FTFont/FTExtrudeFont.cpp',
    'ftgl/src/FTFont/FTFont.cpp',
    'ftgl/src/FTFont/FTFontGlue.cpp',
    'ftgl/src/FTFont/FTOutlineFont.cpp',
    'ftgl/src/FTFont/FTPixmapFont.cpp',
    'ftgl/src/FTFont/FTPolygonFont.cpp',
    'ftgl/src/FTFont/FTTextureFont.cpp',
    'ftgl/src/FTGlyph/FTBitmapGlyph.cpp',
    'ftgl/src/FTGlyph/FTBufferGlyph.cpp',
    'ftgl/src/FTGlyph/FTExtrudeGlyph.cpp',
    'ftgl/src/FTGlyph/FTGlyph.cpp',
    'ftgl/src/FTGlyph/FTGlyphGlue.cpp',
    'ftgl/src/FTGlyph/FTOutlineGlyph.cpp',
    'ftgl/src/FTGlyph/FTPixmapGlyph.cpp',
    'ftgl/src/FTGlyph/FTPolygonGlyph.cpp',
    'ftgl/src/FTGlyph/FTTextureGlyph.cpp',
    'ftgl/src/FTLayout/FTLayout.cpp',
    'ftgl/src/FTLayout/FTLayoutGlue.cpp',
    'ftgl/src/FTLayout/FTSimpleLayout.cpp',
    'ftgl/src/FTCharmap.h',
    'ftgl/src/FTCharToGlyphIndexMap.h',
    'ftgl/src/FTCleanup.h',
    'ftgl/src/FTContour.h',
    'ftgl/src/FTFace.h',
    'ftgl/src/FTGlyphContainer.h',
    'ftgl/src/FTInternals.h',
    'ftgl/src/FTLibrary.h',
    'ftgl/src/FTList.h',
    'ftgl/src/FTSize.h',
    'ftgl/src/FTUnicode.h',
    'ftgl/src/FTVector.h',
    'ftgl/src/FTVectoriser.h',
    'ftgl/src/FTFont/FTBitmapFontImpl.h',
    'ftgl/src/FTFont/FTBufferFontImpl.h',
    'ftgl/src/FTFont/FTExtrudeFontImpl.h',
    'ftgl/src/FTFont/FTFontImpl.h',
    'ftgl/src/FTFont/FTOutlineFontImpl.h',
    'ftgl/src/FTFont/FTPixmapFontImpl.h',
    'ftgl/src/FTFont/FTPolygonFontImpl.h',
    'ftgl/src/FTFont/FTTextureFontImpl.h',
    'ftgl/src/FTGlyph/FTBitmapGlyphImpl.h',
    'ftgl/src/FTGlyph/FTBufferGlyphImpl.h',
    'ftgl/src/FTGlyph/FTExtrudeGlyphImpl.h',
    'ftgl/src/FTGlyph/FTGlyphImpl.h',
    'ftgl/src/FTGlyph/FTOutlineGlyphImpl.h',
    'ftgl/src/FTGlyph/FTPixmapGlyphImpl.h',
    'ftgl/src/FTGlyph/FTPolygonGlyphImpl.h',
    'ftgl/src/FTGlyph/FTTextureGlyphImpl.h',
    'ftgl/src/FTGL/FTBBox.h',
    'ftgl/src/FTGL/FTBitmapGlyph.h',
    'ftgl/src/FTGL/FTBuffer.h',
    'ftgl/src/FTGL/FTBufferFont.h',
    'ftgl/src/FTGL/FTBufferGlyph.h',
    'ftgl/src/FTGL/FTExtrdGlyph.h',
    'ftgl/src/FTGL/FTFont.h',
    'ftgl/src/FTGL/ftgl.h',
    'ftgl/src/FTGL/FTGLBitmapFont.h',
    'ftgl/src/FTGL/FTGLExtrdFont.h',
    'ftgl/src/FTGL/FTGLOutlineFont.h',
    'ftgl/src/FTGL/FTGLPixmapFont.h',
    'ftgl/src/FTGL/FTGLPolygonFont.h',
    'ftgl/src/FTGL/FTGLTextureFont.h',
    'ftgl/src/FTGL/FTGlyph.h',
    'ftgl/src/FTGL/FTLayout.h',
    'ftgl/src/FTGL/FTOutlineGlyph.h',
    'ftgl/src/FTGL/FTPixmapGlyph.h',
    'ftgl/src/FTGL/FTPoint.h',
    'ftgl/src/FTGL/FTPolyGlyph.h',
    'ftgl/src/FTGL/FTSimpleLayout.h',
    'ftgl/src/FTGL/FTTextureGlyph.h',
    'ftgl/src/FTLayout/FTLayoutImpl.h',
    'ftgl/src/FTLayout/FTSimpleLayoutImpl.h',
  }