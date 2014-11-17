  #include "sequence.h"
  #include <iostream>
  #include <fstream>
#include <stdio.h>
#include <string.h>

Sequence::Sequence(const char * seqName, const int start, const int end, const int step)
{
    Name = new char[strlen(seqName)+1];
    strcpy (Name, seqName);

    FirstFrame = start;
    LastFrame = end;
    CurFrame = FirstFrame;
    Step = step;
    Frames = new Tiled_Image*[(LastFrame-FirstFrame)/Step];

    if ((FirstFrame == LastFrame) && (FirstFrame== 0)){
        Frames[0] = new Tiled_Image(seqName);
    }
    else{
        for(unsigned int i = 0; i <= (LastFrame-FirstFrame)/Step; i++) {
            char* TmpName = new char[strlen(seqName)+20]; //on prend la longeur du nom de base auquel on ajoute 20 characteres (c'est un peu crade)
            sprintf(TmpName,seqName,FirstFrame+i*Step);
            Frames[i] = new Tiled_Image(TmpName);
        }
    }
}


Sequence::Sequence(const char * seqName, const int start, const int end)
{
    Sequence(seqName, start, end, 1);
}

Sequence::~Sequence()
{
    delete [] Name;
	for(unsigned int i = 0; i <= (LastFrame-FirstFrame)/Step; i++) {
		delete Frames[i];
	}
    delete [] Frames;
}

void Sequence::SetProxy(char proxy)
{
    for(unsigned int i = 0; i <= (LastFrame-FirstFrame)/Step; i++) {
		((Tiled_Image*)Frames[i])->SetProxy(proxy);
	}
}

char Sequence::GetProxy()
{
    return ((Tiled_Image*)Frames[0])->GetProxy();
}

unsigned int Sequence::GetFrameCount()
{
    return (LastFrame-FirstFrame)/Step+1;
}

void Sequence::Display(unsigned int frame, BYTE* lut)
{
    if ((frame >=0)&&(frame<=(LastFrame-FirstFrame)/Step))
    {
        Tiled_Image* curImage = Frames[frame];
        if (!curImage->Is_Loaded())
            curImage->Load_image();

        if (curImage->Is_Loaded())
            curImage->Display(lut);

    }
}

string Sequence::Get_image_name(unsigned int frame)
{
    if ((frame >=0)&&(frame<=(LastFrame-FirstFrame)/Step))
    {
        Tiled_Image* curImage = Frames[frame];
        return curImage->Get_image_name();
    }
    return NULL;
}
