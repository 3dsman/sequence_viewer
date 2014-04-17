#ifndef TILES
#define TILES

#include <stdlib.h>    // For malloc() etc.
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <glfw3.h>
#include "FreeImage.h"
//  #include "CImg.h"
//  using namespace cimg_library;
using namespace std;
class Tiled_Image
{
public:
       // Constructeur/destructeur

  unsigned short int width;
  unsigned short int height;

  unsigned short int memWidth;
  unsigned short int memHeight;

  Tiled_Image(const char* filename);
  ~Tiled_Image();

  void SetProxy(char proxyLevel);
  char GetProxy();

  void Tile_Image(FIBITMAP* input);
  void Load_image ();
  void Display(BYTE* lut);


  bool Is_Loaded();
  string Get_image_name();
private:

  FIBITMAP* _image;
  BYTE * _imageDataPtr;
  short int tileSize;
  char proxy;

  string  ImageName;//[ 256 ];
  //GLFWthread allocationthread;
  bool FileExists( const char* FileName );
};
#endif
