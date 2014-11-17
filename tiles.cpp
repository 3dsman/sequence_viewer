
//#define FREEIMAGE_LIB
  #include <GL/gl.h>
  #include <GL/glext.h>
  #include "tiles.h"
#include <stdio.h>
#include <string.h>
#include "FreeImage.h"


Tiled_Image::Tiled_Image (const char* filename)
{
    //strcpy (ImageName, filename);
    ImageName = filename;
    _image = NULL;
    _imageDataPtr = NULL;
    proxy = 1;
}

Tiled_Image::~Tiled_Image()
{
    if(_image != NULL)
    {
        FreeImage_Unload(_image);
    }
};

void Tiled_Image::SetProxy(char proxyLevel)
{
    if (proxyLevel>=1)
    {
        if(_image != NULL)
        {
            FreeImage_Unload(_image);
            _image = NULL;
        }
        proxy = proxyLevel;
    }
}

char Tiled_Image::GetProxy()
{
    return proxy;
}

bool Tiled_Image::FileExists( const char* FileName )
{
    FILE* fp = NULL;

    fp = fopen( FileName, "rb" );
    if( fp != NULL )
    {
        fclose( fp );
        return true;
    }
    else
        return false;
}

void Tiled_Image::Tile_Image (FIBITMAP* input)
{
    FIBITMAP *dib = FreeImage_ConvertTo24Bits(input);
    if (proxy!=1)
    {
        FIBITMAP *dib2 = FreeImage_Rescale(dib,FreeImage_GetWidth(dib)/proxy,FreeImage_GetHeight(dib)/proxy,FILTER_BOX);
        FreeImage_Unload(dib);
        dib = dib2;
    }

//    FIBITMAP *dib = input;
    width = FreeImage_GetWidth(dib)*proxy;
    height = FreeImage_GetHeight(dib)*proxy;
    tileSize = 64;

    int addWidth = (tileSize-width/proxy%tileSize)%tileSize;
    int addHeight = (tileSize-height/proxy%tileSize)%tileSize;

    _image = FreeImage_Allocate(addWidth+width/proxy, addHeight+height/proxy, 24);
    FreeImage_Paste(_image, dib, 0,addHeight,255);
    FreeImage_Unload(dib);

    memWidth = FreeImage_GetWidth(_image);
    memHeight = FreeImage_GetHeight(_image);

    _imageDataPtr = (BYTE*)FreeImage_GetBits(_image);

};

void Tiled_Image::Load_image ()
{
    if ((ImageName.size()>0)&&(FileExists(ImageName.c_str())))
    {
     	// Get the image file type from FreeImage.
        FREE_IMAGE_FORMAT fifmt = FreeImage_GetFileType(ImageName.c_str(), 0);

        // Actually load the image file.
        FIBITMAP *texture = FreeImage_Load(fifmt, ImageName.c_str(),0);

        FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(texture);
        FIBITMAP *dib = NULL;
        switch(image_type)
        {
            case FIT_UINT16:
            case FIT_UINT32:
            case FIT_INT16:
            case FIT_INT32:
            case FIT_RGB16:
            case FIT_RGBA16:
            case FIT_BITMAP:
                dib = FreeImage_ConvertTo24Bits(texture);
                FreeImage_Unload(texture);
                texture = dib;
            case FIT_FLOAT:
            case FIT_DOUBLE:
            case FIT_COMPLEX:
                dib = FreeImage_ConvertToStandardType(texture, TRUE);
                FreeImage_Unload(texture);
                texture = dib;
                break;
            case FIT_RGBF:
            case FIT_RGBAF:
                dib = FreeImage_ToneMapping(texture, FITMO_DRAGO03);
                FreeImage_Unload(texture);
                texture = dib;
                break;
                // no way to keep the transparency yet ... anyway tone mapping destroy alpha channel
                /*FIBITMAP *rgbf = FreeImage_ConvertToRGBF(texture);
                dib = FreeImage_ToneMapping(rgbf, FITMO_REINHARD05);
                FreeImage_Unload(rgbf);
                FreeImage_Unload(texture);
                texture = dib;
                break;*/
            case FIT_UNKNOWN:
                break;
        }
        Tile_Image (texture);
        FreeImage_Unload(texture);
    }
}

void Tiled_Image::Display(BYTE* lut)
{
    FIBITMAP *tmpImage = FreeImage_Clone(_image);
    if (lut!=0)FreeImage_AdjustCurve(tmpImage, lut, FICC_RGB);

    glTranslatef(-width/2,-height/2,0 );
    glEnable( GL_TEXTURE_2D );

    glPushMatrix();

    glColor3f(1.0,1.0,1.0);

    double nbTilesx = (double)width/proxy/(double)tileSize;
    double nbTilesy = (double)height/proxy/(double)tileSize;

    BYTE *pixels = (BYTE*)FreeImage_GetBits(tmpImage);
    BYTE *curPixels;

    glPixelStorei(GL_UNPACK_ROW_LENGTH,memWidth);
    glScalef(tileSize*proxy,tileSize*proxy,0);

    float valx,valy;
    for (int y=0; y<nbTilesy; y++)
    {
        valy = nbTilesy-y;
	    if (valy>1.0) valy = 1.0;
        for (int x=0; x<nbTilesx; x++)
        {
           curPixels = pixels + (x+y*memWidth)*tileSize*3;
	       glTexImage2D( GL_TEXTURE_2D, 0, 3, tileSize, tileSize, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, curPixels );
	       valx = nbTilesx-x;
	       if (valx>1.0) valx = 1.0;

           glBegin( GL_QUADS );          // Tell OpenGL that we want to draw a quad
           glTexCoord2f(0,0);
           glVertex3f( x, y, 0.0f ); // First corner of the quad
           glTexCoord2f(0,valy);
           glVertex3f(  x, y+valy, 0.0f ); // Second corner of the quad
           glTexCoord2f(valx,valy);
           glVertex3f(  x+valx,  y+valy, 0.0f ); // Third corner of the quad
           glTexCoord2f(valx,0);
           glVertex3f(  x+valx,  y, 0.0f ); // Last corner of the quad
           glEnd();

        }
     }

    glPopMatrix();

    glDisable( GL_TEXTURE_2D );
              glLineWidth(2);
            glBegin(GL_LINE_LOOP);
                  glColor3f(1.0,0.0,0.0);
                  glVertex3i( 0, 0, 1 );
                  glVertex3i(  width, 0, 1 );
                  glVertex3i(  width,  height, 1 );
                  glVertex3i( 0, height, 1  );
            glEnd();
            glTranslatef(width/2,height/2,0 );
    FreeImage_Unload(tmpImage);
};

bool Tiled_Image::Is_Loaded()
{
    if (_image!=NULL)
        return true;
    else
        return false;
}

string Tiled_Image::Get_image_name()
{
    return ImageName;
}
