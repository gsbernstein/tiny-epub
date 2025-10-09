# Tiny EPUB

A minimal and display/font-agnostic EPUB library targeting embedded MCUs. Everything with a screen and a C++ compiler can be an eBook!

This library has two main components:

## `epubfile`

Responsible for unzipping the EPUB in-place (no need to unzip the full thing) and navigating through the EPUB contents.

## `renderer`

Text layout engine for the book. This puts the words in the right spots on a page. In addition to EPUB files, this also supports plaintext.

## Goals

* Embedded friendly - very small binary, minimal CPU
* Font-agnostic - bring your own text drawing code. See `RendererDisplay.hpp` for the required interface.
* Display-agnostic - relatively unopinionated about the underlying screen technology.

## Limitations

* No CSS support
* Only tested against EPUB 3 formatted EPUBs
* Image support is limited to grayscale BMPs (for now!)

## See also

* [tiny-font](https://github.com/sindarin-inc/tiny-font) embedded font library.
