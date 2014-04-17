#ifndef SEQUENCE
#define SEQUENCE

#include "tiles.h"
  //using namespace cimg_library;
class Sequence
{
public:
       // Constructeur/destructeur

  unsigned int FirstFrame;
  unsigned int LastFrame;
  unsigned int CurFrame;
  unsigned int Step;
  char * Name;
  Tiled_Image **Frames;

  Sequence(void);
  Sequence(const char * seqName, const int start, const int end, const int step);
  Sequence(const char * seqName, const int start, const int end);
  ~Sequence();

  void SetProxy(char proxy);
  char GetProxy();
  unsigned int GetFrameCount();
  void Display(unsigned int frame, BYTE* lut);
  string Get_image_name(unsigned int frame);
};

#endif
