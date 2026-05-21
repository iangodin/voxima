#! /usr/bin/env python3

output_dir = "assets/fonts"

font_renames = {
  "IBM Plex Serif Var-Italic.ttf": "IBMPlexSerif-Italic-Variable.ttf",
  "IBM Plex Serif Var-Roman.ttf": "IBMPlexSerif-Variable.ttf",
  "IBM Plex Sans Var-Roman.ttf": "IBMPlexSans-Variable.ttf",
  "IBM Plex Sans Var-Italic.ttf": "IBMPlexSans-Italic-Variable.ttf",
  "Comfortaa%5Bwght%5D.ttf": "Comfortaa-Variable.ttf",
  "NotoSerifCJK-VF.ttf.ttc": "NotoSerifCJK-Variable.ttc",
  "NotoSansMonoCJK-VF.ttf.ttc": "NonotoSansMonoCJK-Variable.ttc",
  "NotoSansCJK-VF.ttf.ttc": "NonotoSansCJK-Variable.ttc",
  "NotoEmoji%5Bwght%5D.ttf": "NotoEmoji-Variable.ttf",
  "Quicksand%5Bwght%5D.ttf": "Quicksand-Variable.ttf",
}

import os
import io
import zipfile
import requests
import posixpath
from urllib.parse import urlparse

os.makedirs(output_dir, exist_ok=True)

def download_file(name, theurl, files = None):
  print(f"Downloading {name}")

  urls = []

  if files is None:
    urls = [theurl]
  else:
    urls = [ theurl + file for file in files ]

  for url in urls:
    response = requests.get(url)
    response.raise_for_status()
    filename = urlparse(url).path.split("/")[-1]
    if filename in font_renames:
      filename = font_renames[filename]
    destination = os.path.join(output_dir, filename)
    with open(destination, "wb") as dest_file:
      dest_file.write(response.content)
    print(f"  Saved {filename}")

def download_zip(name, url, files = None):
  print(f"Downloading {name}")
  response = requests.get(url)
  response.raise_for_status()

  zip_buffer = io.BytesIO(response.content)
  with zipfile.ZipFile(zip_buffer) as the_zip:
    if files is None:
      for filename in the_zip.namelist():
        print(f"  File {filename}")
      return

    for file in files:
      filename = os.path.basename(file)
      if filename in font_renames:
        filename = font_renames[filename]
      destination = os.path.join(output_dir, filename)
      with the_zip.open(file) as source_file:
        with open(destination, "wb") as dest_file:
          dest_file.write(source_file.read())
      print(f"  Saved {filename}")

download_zip("Amiri", "https://github.com/aliftype/amiri/releases/download/1.003/Amiri-1.003.zip", [
  "Amiri-1.003/Amiri-Regular.ttf",
  "Amiri-1.003/Amiri-Bold.ttf",
  "Amiri-1.003/Amiri-Italic.ttf",
  "Amiri-1.003/Amiri-BoldItalic.ttf"])

download_zip("Adobe Not-Defined", "https://github.com/adobe-fonts/adobe-notdef/archive/refs/tags/1.001.zip", [
  "adobe-notdef-1.001/AND-Regular.otf"])

download_file("Comfortaa", "https://github.com/googlefonts/comfortaa/raw/refs/heads/main/fonts/variable/Comfortaa%5Bwght%5D.ttf")

download_zip("DejaVu", "https://github.com/dejavu-fonts/dejavu-fonts/releases/download/version_2_37/dejavu-fonts-ttf-2.37.zip", [
  "dejavu-fonts-ttf-2.37/ttf/DejaVuSansMono-Oblique.ttf",
  "dejavu-fonts-ttf-2.37/ttf/DejaVuSansMono-BoldOblique.ttf",
  "dejavu-fonts-ttf-2.37/ttf/DejaVuSansMono.ttf",
  "dejavu-fonts-ttf-2.37/ttf/DejaVuSansMono-Bold.ttf"]);

download_zip("Fira Code", "https://github.com/tonsky/FiraCode/releases/download/6.2/Fira_Code_v6.2.zip", [
  "ttf/FiraCode-Bold.ttf",
  "ttf/FiraCode-Light.ttf",
  "ttf/FiraCode-Medium.ttf",
  "ttf/FiraCode-Regular.ttf",
  "ttf/FiraCode-Retina.ttf",
  "ttf/FiraCode-SemiBold.ttf"])

download_zip("Fira GO", "https://carrois.com/downloads/FiraGO/Download_Folder_FiraGO_1001.zip", [
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-Bold.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-Book.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-Eight.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-ExtraBold.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-ExtraLight.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-Four.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-Hair.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-Heavy.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-Light.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-Medium.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-Regular.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-SemiBold.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-Thin.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-Two.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Roman/FiraGO-UltraLight.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-BoldItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-BookItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-EightItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-ExtraBoldItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-ExtraLightItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-FourItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-HairItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-HeavyItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-Italic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-LightItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-MediumItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-SemiBoldItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-ThinItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-TwoItalic.ttf",
  "Download_Folder_FiraGO_1001/Fonts/FiraGO_TTF_1001/Italic/FiraGO-UltraLightItalic.ttf"])

download_file("Fira Math", "https://github.com/firamath/firamath/releases/download/v0.3.4/FiraMath-Regular.otf")

download_zip("Fira Mono", "https://carrois.com/downloads/Fira/Fira_Mono_3_2.zip", [
  "Fira_Mono_3_2/Fonts/FiraMono_WEB_32/FiraMono-Bold.ttf",
  "Fira_Mono_3_2/Fonts/FiraMono_WEB_32/FiraMono-Medium.ttf",
  "Fira_Mono_3_2/Fonts/FiraMono_WEB_32/FiraMono-Regular.ttf"])

download_zip("Fira Sans", "https://carrois.com/downloads/Fira/Download_Folder_FiraSans_4301.zip", [
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-BoldItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-BookItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-EightItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-ExtraBoldItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-ExtraLightItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-FourItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-HairItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-HeavyItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-Italic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-LightItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-MediumItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-SemiBoldItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-ThinItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-TwoItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Italic/FiraSansCompressed-UltraLightItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-Bold.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-Book.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-Eight.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-ExtraBold.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-ExtraLight.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-Four.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-Hair.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-Heavy.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-Light.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-Medium.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-Regular.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-SemiBold.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-Thin.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-Two.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Compressed/Roman/FiraSansCompressed-UltraLight.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-BoldItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-BookItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-EightItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-ExtraBoldItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-ExtraLightItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-FourItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-HairItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-HeavyItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-Italic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-LightItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-MediumItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-SemiBoldItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-ThinItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-TwoItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Italic/FiraSansCondensed-UltraLightItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-Bold.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-Book.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-Eight.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-ExtraBold.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-ExtraLight.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-Four.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-Hair.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-Heavy.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-Light.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-Medium.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-Regular.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-SemiBold.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-Thin.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-Two.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Condensed/Roman/FiraSansCondensed-UltraLight.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-BoldItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-BookItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-EightItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-ExtraBoldItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-ExtraLightItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-FourItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-HairItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-HeavyItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-Italic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-LightItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-MediumItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-SemiBoldItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-ThinItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-TwoItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-UltraItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Italic/FiraSans-UltraLightItalic.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-Bold.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-Book.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-Eight.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-ExtraBold.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-ExtraLight.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-Four.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-Hair.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-Heavy.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-Light.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-Medium.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-Regular.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-SemiBold.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-Thin.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-Two.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-Ultra.ttf",
  "Download_Folder_FiraSans_4301/Fonts/Fira_Sans_TTF_4301/Normal/Roman/FiraSans-UltraLight.ttf"])

download_zip("GFS Neohellenic Math", "https://mirrors.ctan.org/fonts/gfsneohellenicmath.zip", [
  "gfsneohellenicmath/GFSNeohellenicMath.otf"])

download_zip("IBM Plex", "https://github.com/IBM/plex/releases/download/v6.0.2/TrueType.zip", [
  "TrueType/IBM-Plex-Sans-JP/hinted/IBMPlexSansJP-Bold.ttf",
  "TrueType/IBM-Plex-Sans-JP/hinted/IBMPlexSansJP-Thin.ttf",
  "TrueType/IBM-Plex-Sans-JP/hinted/IBMPlexSansJP-ExtraLight.ttf",
  "TrueType/IBM-Plex-Sans-JP/hinted/IBMPlexSansJP-Medium.ttf",
  "TrueType/IBM-Plex-Sans-JP/hinted/IBMPlexSansJP-SemiBold.ttf",
  "TrueType/IBM-Plex-Sans-JP/hinted/IBMPlexSansJP-Light.ttf",
  "TrueType/IBM-Plex-Sans-JP/hinted/IBMPlexSansJP-Regular.ttf",
  "TrueType/IBM-Plex-Sans-JP/hinted/IBMPlexSansJP-Text.ttf",
  "TrueType/IBM-Plex-Sans-Arabic/IBMPlexSansArabic-SemiBold.ttf",
  "TrueType/IBM-Plex-Sans-Arabic/IBMPlexSansArabic-Regular.ttf",
  "TrueType/IBM-Plex-Sans-Arabic/IBMPlexSansArabic-Light.ttf",
  "TrueType/IBM-Plex-Sans-Arabic/IBMPlexSansArabic-Text.ttf",
  "TrueType/IBM-Plex-Sans-Arabic/IBMPlexSansArabic-ExtraLight.ttf",
  "TrueType/IBM-Plex-Sans-Arabic/IBMPlexSansArabic-Bold.ttf",
  "TrueType/IBM-Plex-Sans-Arabic/IBMPlexSansArabic-Thin.ttf",
  "TrueType/IBM-Plex-Sans-Arabic/IBMPlexSansArabic-Medium.ttf",
  "TrueType/IBM-Plex-Sans-Hebrew/IBMPlexSansHebrew-Text.ttf",
  "TrueType/IBM-Plex-Sans-Hebrew/IBMPlexSansHebrew-Regular.ttf",
  "TrueType/IBM-Plex-Sans-Hebrew/IBMPlexSansHebrew-Light.ttf",
  "TrueType/IBM-Plex-Sans-Hebrew/IBMPlexSansHebrew-SemiBold.ttf",
  "TrueType/IBM-Plex-Sans-Hebrew/IBMPlexSansHebrew-Thin.ttf",
  "TrueType/IBM-Plex-Sans-Hebrew/IBMPlexSansHebrew-Bold.ttf",
  "TrueType/IBM-Plex-Sans-Hebrew/IBMPlexSansHebrew-Medium.ttf",
  "TrueType/IBM-Plex-Sans-Hebrew/IBMPlexSansHebrew-ExtraLight.ttf",
  "TrueType/IBM-Plex-Sans-KR/hinted/IBMPlexSansKR-Text.ttf",
  "TrueType/IBM-Plex-Sans-KR/hinted/IBMPlexSansKR-ExtraLight.ttf",
  "TrueType/IBM-Plex-Sans-KR/hinted/IBMPlexSansKR-Regular.ttf",
  "TrueType/IBM-Plex-Sans-KR/hinted/IBMPlexSansKR-SemiBold.ttf",
  "TrueType/IBM-Plex-Sans-KR/hinted/IBMPlexSansKR-Medium.ttf",
  "TrueType/IBM-Plex-Sans-KR/hinted/IBMPlexSansKR-Light.ttf",
  "TrueType/IBM-Plex-Sans-KR/hinted/IBMPlexSansKR-Thin.ttf",
  "TrueType/IBM-Plex-Sans-KR/hinted/IBMPlexSansKR-Bold.ttf",
  "TrueType/IBM-Plex-Sans-Thai/IBMPlexSansThai-Regular.ttf",
  "TrueType/IBM-Plex-Sans-Thai/IBMPlexSansThai-Bold.ttf",
  "TrueType/IBM-Plex-Sans-Thai/IBMPlexSansThai-Medium.ttf",
  "TrueType/IBM-Plex-Sans-Thai/IBMPlexSansThai-Thin.ttf",
  "TrueType/IBM-Plex-Sans-Thai/IBMPlexSansThai-Light.ttf",
  "TrueType/IBM-Plex-Sans-Thai/IBMPlexSansThai-Text.ttf",
  "TrueType/IBM-Plex-Sans-Thai/IBMPlexSansThai-ExtraLight.ttf",
  "TrueType/IBM-Plex-Sans-Thai/IBMPlexSansThai-SemiBold.ttf",
  "TrueType/IBM-Plex-Sans-Thai-Looped/IBMPlexSansThaiLooped-Thin.ttf",
  "TrueType/IBM-Plex-Sans-Thai-Looped/IBMPlexSansThaiLooped-Medium.ttf",
  "TrueType/IBM-Plex-Sans-Thai-Looped/IBMPlexSansThaiLooped-Bold.ttf",
  "TrueType/IBM-Plex-Sans-Thai-Looped/IBMPlexSansThaiLooped-Text.ttf",
  "TrueType/IBM-Plex-Sans-Thai-Looped/IBMPlexSansThaiLooped-SemiBold.ttf",
  "TrueType/IBM-Plex-Sans-Thai-Looped/IBMPlexSansThaiLooped-Regular.ttf",
  "TrueType/IBM-Plex-Sans-Thai-Looped/IBMPlexSansThaiLooped-Light.ttf",
  "TrueType/IBM-Plex-Sans-Thai-Looped/IBMPlexSansThaiLooped-ExtraLight.ttf",
  "TrueType/IBM-Plex-Sans-Devanagari/IBMPlexSansDevanagari-Text.ttf",
  "TrueType/IBM-Plex-Sans-Devanagari/IBMPlexSansDevanagari-Medium.ttf",
  "TrueType/IBM-Plex-Sans-Devanagari/IBMPlexSansDevanagari-ExtraLight.ttf",
  "TrueType/IBM-Plex-Sans-Devanagari/IBMPlexSansDevanagari-Regular.ttf",
  "TrueType/IBM-Plex-Sans-Devanagari/IBMPlexSansDevanagari-Bold.ttf",
  "TrueType/IBM-Plex-Sans-Devanagari/IBMPlexSansDevanagari-Thin.ttf",
  "TrueType/IBM-Plex-Sans-Devanagari/IBMPlexSansDevanagari-SemiBold.ttf",
  "TrueType/IBM-Plex-Sans-Devanagari/IBMPlexSansDevanagari-Light.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-MediumItalic.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-Italic.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-Bold.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-Thin.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-Light.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-Text.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-LightItalic.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-ExtraLight.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-ThinItalic.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-BoldItalic.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-SemiBoldItalic.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-ExtraLightItalic.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-Regular.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-SemiBold.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-TextItalic.ttf",
  "TrueType/IBM-Plex-Sans-Condensed/IBMPlexSansCondensed-Medium.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-TextItalic.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-Thin.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-Bold.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-MediumItalic.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-BoldItalic.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-Medium.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-ExtraLight.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-ExtraLightItalic.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-ThinItalic.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-Regular.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-Light.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-Italic.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-Text.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-SemiBold.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-SemiBoldItalic.ttf",
  "TrueType/IBM-Plex-Mono/IBMPlexMono-LightItalic.ttf"])

download_zip("IBM Plex Serif", "https://github.com/IBM/plex/releases/download/%40ibm%2Fplex-serif-variable%402.0.0/plex-serif-variable.zip", [
  "plex-serif-variable/complete/ttf/IBM Plex Serif Var-Italic.ttf",
  "plex-serif-variable/complete/ttf/IBM Plex Serif Var-Roman.ttf"])

download_zip("IBM Plex Sans", "https://github.com/IBM/plex/releases/download/%40ibm%2Fplex-sans-variable%400.2.0/plex-sans-variable.zip", [
  "fonts/complete/ttf/IBM Plex Sans Var-Roman.ttf",
  "fonts/complete/ttf/IBM Plex Sans Var-Italic.ttf"])

download_zip("IBM Plex Math", "https://github.com/IBM/plex/releases/download/%40ibm%2Fplex-math%401.1.0/ibm-plex-math.zip", [
  "ibm-plex-math/fonts/complete/ttf/IBMPlexMath-Regular.ttf"])

download_file("Kosugi Maru", "https://github.com/googlefonts/kosugi-maru/raw/refs/heads/main/fonts/ttf/KosugiMaru-Regular.ttf")

download_zip("Noto Serif CJK", "https://github.com/notofonts/noto-cjk/releases/download/Serif2.003/03_NotoSerifCJK-TTF-VF.zip", [
  "Variable/OTC/NotoSerifCJK-VF.ttf.ttc"])

download_zip("Noto Sans CJK", "https://github.com/notofonts/noto-cjk/releases/download/Sans2.004/02_NotoSansCJK-TTF-VF.zip", [
  "Variable/OTC/NotoSansCJK-VF.ttf.ttc",
  "Variable/OTC/NotoSansMonoCJK-VF.ttf.ttc"])

download_file("Noto Emoji", "https://github.com/google/fonts/raw/refs/heads/main/ofl/notoemoji/NotoEmoji%5Bwght%5D.ttf")

download_file( "Quicksand", "https://github.com/andrew-paglinawan/QuicksandFamily/raw/refs/heads/master/fonts/Quicksand%5Bwght%5D.ttf" )

download_zip("Source Hans Serif", "https://github.com/adobe-fonts/source-han-serif/releases/download/2.003R/01_SourceHanSerif.ttc.zip", [
  "SourceHanSerif.ttc"])

download_zip("Stix Two", "https://github.com/stipub/stixfonts/archive/refs/tags/v2.13b171.zip", [
  "stixfonts-2.13b171/archive/STIXv2.0.2/OTF/STIX2Math.otf",
  "stixfonts-2.13b171/archive/STIXv2.0.2/OTF/STIX2Text-Bold.otf",
  "stixfonts-2.13b171/archive/STIXv2.0.2/OTF/STIX2Text-BoldItalic.otf",
  "stixfonts-2.13b171/archive/STIXv2.0.2/OTF/STIX2Text-Italic.otf",
  "stixfonts-2.13b171/archive/STIXv2.0.2/OTF/STIX2Text-Regular.otf"])

download_file("Tajawal", "https://github.com/googlefonts/tajawal/raw/refs/heads/main/fonts/ttf/", [
  "Tajawal-Black.ttf",
  "Tajawal-Bold.ttf",
  "Tajawal-ExtraBold.ttf",
  "Tajawal-ExtraLight.ttf",
  "Tajawal-Light.ttf",
  "Tajawal-Medium.ttf",
  "Tajawal-Regular.ttf"])

download_file("Symbola Emoji", "https://github.com/ChALkeR/Symbola-Emoji/raw/refs/heads/master/dist/Symbola-Emoji.ttf")

download_file("New Computer Modern", "https://mirrors.ctan.org/fonts/newcomputermodern/otf/", [
  "NewCM08-Book.otf",
  "NewCM08-BookItalic.otf",
  "NewCM08-Italic.otf",
  "NewCM08-Regular.otf",
  "NewCM08Devanagari-Book.otf",
  "NewCM08Devanagari-Regular.otf",
  "NewCM10-Bold.otf",
  "NewCM10-BoldItalic.otf",
  "NewCM10-Book.otf",
  "NewCM10-BookItalic.otf",
  "NewCM10-Italic.otf",
  "NewCM10-Regular.otf",
  "NewCM10Devanagari-Book.otf",
  "NewCM10Devanagari-Regular.otf",
  "NewCMMath-Bold.otf",
  "NewCMMath-Book.otf",
  "NewCMMath-Regular.otf",
  "NewCMMono10-Bold.otf",
  "NewCMMono10-BoldOblique.otf",
  "NewCMMono10-Book.otf",
  "NewCMMono10-BookItalic.otf",
  "NewCMMono10-Italic.otf",
  "NewCMMono10-Regular.otf",
  "NewCMSans08-Book.otf",
  "NewCMSans08-BookOblique.otf",
  "NewCMSans08-Oblique.otf",
  "NewCMSans08-Regular.otf",
  "NewCMSans10-Bold.otf",
  "NewCMSans10-BoldOblique.otf",
  "NewCMSans10-Book.otf",
  "NewCMSans10-BookOblique.otf",
  "NewCMSans10-Oblique.otf",
  "NewCMSans10-Regular.otf",
  "NewCMSansMath-Regular.otf",
  "NewCMUncial08-Bold.otf",
  "NewCMUncial08-Book.otf",
  "NewCMUncial08-Regular.otf",
  "NewCMUncial10-Bold.otf",
  "NewCMUncial10-Book.otf",
  "NewCMUncial10-Regular.otf"])

