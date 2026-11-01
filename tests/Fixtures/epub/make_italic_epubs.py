# Generates italics.epub and wrapping.epub, used by the [italic] renderer tests.
# Run from this directory: python3 make_italic_epubs.py
import zipfile

CONTAINER = """<?xml version="1.0"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
   <rootfiles>
      <rootfile full-path="metadata.opf" media-type="application/oebps-package+xml"/>
   </rootfiles>
</container>
"""


def write_epub(path, uid, title, body):
    chapter = f"""<?xml version="1.0" encoding="utf-8"?>
<html xmlns="http://www.w3.org/1999/xhtml">
<head><title>{title}</title></head>
<body>
{body}
</body>
</html>
"""
    opf = f"""<?xml version='1.0' encoding='utf-8'?>
<package xmlns="http://www.idpf.org/2007/opf" unique-identifier="uuid_id" version="2.0">
  <metadata xmlns:dc="http://purl.org/dc/elements/1.1/">
    <dc:identifier id="uuid_id">{uid}</dc:identifier>
    <dc:title>{title}</dc:title>
    <dc:creator>Sol Team</dc:creator>
    <dc:language>en</dc:language>
  </metadata>
  <manifest>
    <item href="chapter.xhtml" id="chapter" media-type="application/xhtml+xml"/>
    <item href="toc.ncx" id="ncx" media-type="application/x-dtbncx+xml"/>
  </manifest>
  <spine toc="ncx">
    <itemref idref="chapter"/>
  </spine>
</package>
"""
    ncx = f"""<?xml version='1.0' encoding='utf-8'?>
<ncx xmlns="http://www.daisy.org/z3986/2005/ncx/" version="2005-1">
  <head><meta name="dtb:uid" content="{uid}"/></head>
  <docTitle><text>{title}</text></docTitle>
  <navMap>
    <navPoint id="chapter" playOrder="1"><navLabel><text>{title}</text></navLabel><content src="chapter.xhtml"/></navPoint>
  </navMap>
</ncx>
"""
    with zipfile.ZipFile(path, "w") as z:
        z.writestr(zipfile.ZipInfo("mimetype"), "application/epub+zip",
                   compress_type=zipfile.ZIP_STORED)
        for name, data in [("META-INF/container.xml", CONTAINER), ("metadata.opf", opf),
                           ("toc.ncx", ncx), ("chapter.xhtml", chapter)]:
            info = zipfile.ZipInfo(name)
            z.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED)


ITALICS_BODY = """<p>Plain <i>The <em>Odyssey</em> was long</i> afterward.</p>
<p><span class="italic">outer <span class="italic">inner</span> tail</span> done</p>
<p><span class="calibre italic">alpha <em>beta</em> gamma</span> delta</p>
<p><i>first <span class="bold">second</span> third</i> fourth</p>
<p><i>note<sup>2</sup> ending</i> closing</p>
<p>value<sup>sa<i>sb</i>sc</sup> trailer</p>
<p><strong>strongword</strong> <b>boldword</b> regular</p>
<p><a href="#n1">See <i>linknote</i></a> after1</p>
<p><a href="#n2">plain linked</a> after2</p>
<p><a href="#n3">trailing <em>spaced</em> </a>after3</p>"""

WORDS = ["alpha", "bravo", "charlie", "delta", "echo", "foxtrot", "golf", "hotel", "india",
         "juliet", "kilo", "lima", "mike", "november", "oscar", "papa", "quebec", "romeo",
         "sierra", "tango", "uniform", "victor", "whiskey", "xray", "yankee", "zulu"]


def wrapping_body():
    # One long paragraph where many words are split into several tokens without a space between
    # them (style changes, footnote markers), so some of them land at the end of a line.
    parts = []
    for i in range(260):
        w = WORDS[(i * 7) % len(WORDS)]
        w2 = WORDS[(i * 11 + 3) % len(WORDS)]
        form = i % 7
        if form == 0:
            parts.append(f"<i>{w}</i>,")
        elif form == 1:
            parts.append(w)
        elif form == 2:
            parts.append(f"<i>{w}</i>.")
        elif form == 3:
            parts.append(f"{w}<i>ness</i>")
        elif form == 4:
            parts.append(f"{w}<sup>{i % 9 + 1}</sup>")
        elif form == 5:
            parts.append(f'<span class="italic">{w}</span>;')
        else:
            parts.append(f"{w}-<i>{w2}</i>")
    return "<p>" + " ".join(parts) + "</p>"


write_epub("italics.epub", "8c6a2f4e-5d1b-4c0a-9e3f-italics000001", "Italics", ITALICS_BODY)
write_epub("wrapping.epub", "8c6a2f4e-5d1b-4c0a-9e3f-wrapping00001", "Wrapping",
           wrapping_body())
