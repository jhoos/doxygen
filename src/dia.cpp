/******************************************************************************
 *
 * 
 *
 * Copyright (C) 1997-2014 by Dimitri van Heesch.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License is hereby 
 * granted. No representations are made about the suitability of this software 
 * for any purpose. It is provided "as is" without express or implied warranty.
 * See the GNU General Public License for more details.
 *
 * Documents produced by Doxygen are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
 */

#include "dia.h"
#include "portable.h"
#include "config.h"
#include "message.h"
#include "util.h"

#include <qdir.h>
#include <math.h>

static const int maxCmdLine = 40960;

// extract size from a dot generated SVG file
static bool readSVGSize(const QCString &fileName,int *width,int *height)
{
  bool found=FALSE;
  QFile f(fileName);
  int ppi = Config_getInt("DIA_SVG_PPI");
  if (!f.open(IO_ReadOnly))
  {
    return FALSE;
  }
  const int maxLineLen=4096;
  char buf[maxLineLen];
  while (!f.atEnd() && !found)
  {
    int numBytes = f.readLine(buf,maxLineLen-1); // read line
    if (numBytes>0)
    {
      buf[numBytes]='\0';
      if (sscanf(buf,"<svg width=\"%dcm\" height=\"%dcm\"",width,height)==2)
      {
        // convert from cm
        *width=ceil(*width/2.54*ppi);
        *height=ceil(*height/2.54*ppi);
        //printf("Found fixed size %dx%d for %s!\n",*width,*height,fileName.data());
        found=TRUE;
      }
    }
    else // read error!
    {
      //printf("Read error %d!\n",numBytes);
      return FALSE;
    }
  }
  return TRUE;
}

// check if a reference to a SVG figure can be written and does so if possible.
// return FALSE if not possible (for instance because the SVG file is not yet generated).
bool writeDiaSVGFigureLink(FTextStream &out,const QCString &relPath,
                           const QCString &baseName,const QCString &absImgName)
{
  int width=600,height=600;
  if (!readSVGSize(absImgName,&width,&height))
  {
    return FALSE;
  }

  out << "<img src=\""
      << relPath << baseName << ".svg\" width=\""
      << width << "\" height=\"" << height << "\">";
  out << "</object>";
  if (width==-1)
  {
    out << "</div>";
  }

  return TRUE;
}

void writeDiaGraphFromFile(const char *inFile,const char *outDir,
                           const char *outFile,DiaOutputFormat format)
{
  QCString absOutFile = outDir;
  absOutFile+=portable_pathSeparator();
  absOutFile+=outFile;

  QCString diaExe = Config_getString("DIA_PATH")+"dia"+portable_commandExtension();
  QCString diaArgs;
  QCString extension;
  diaArgs+="-n ";
  if (format==DIA_BITMAP)
  {
    diaArgs+="-t png-libart";
    extension=".png";
  }
  else if (format==DIA_EPS)
  {
    diaArgs+="-t eps";
    extension=".eps";
  }
  else if (format==DIA_SVG)
  {
    diaArgs+="-t svg";
    extension=".svg";
  }

  diaArgs+=" -e \"";
  diaArgs+=absOutFile;
  diaArgs+=extension+"\"";

  diaArgs+=" \"";
  diaArgs+=inFile;
  diaArgs+="\"";

  int exitCode;
  //printf("*** running: %s %s outDir:%s %s\n",diaExe.data(),diaArgs.data(),outDir,outFile);
  portable_sysTimerStart();
  if ((exitCode=portable_system(diaExe,diaArgs,FALSE))!=0)
  {
    portable_sysTimerStop();
    goto error;
  }
  portable_sysTimerStop();
  if ( (format==DIA_EPS) && (Config_getBool("USE_PDFLATEX")) )
  {
    QCString epstopdfArgs(maxCmdLine);
    epstopdfArgs.sprintf("\"%s.eps\" --outfile=\"%s.pdf\"",
                         absOutFile.data(),absOutFile.data());
    portable_sysTimerStart();
    if (portable_system("epstopdf",epstopdfArgs)!=0)
    {
      err("Problems running epstopdf. Check your TeX installation!\n");
    }
    portable_sysTimerStop();
  }

error:
  return;
}

