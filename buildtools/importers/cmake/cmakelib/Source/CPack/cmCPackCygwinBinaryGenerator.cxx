/*=========================================================================

  Program:   CMake - Cross-Platform Makefile Generator
  Module:    $RCSfile: cmCPackCygwinBinaryGenerator.cxx,v $
  Language:  C++
  Date:      $Date: 2006/06/09 17:45:09 $
  Version:   $Revision: 1.1 $

  Copyright (c) 2002 Kitware, Inc., Insight Consortium.  All rights reserved.
  See Copyright.txt or http://www.cmake.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include "cmCPackCygwinBinaryGenerator.h"

#include "cmake.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmSystemTools.h"
#include "cmMakefile.h"
#include "cmGeneratedFileStream.h"
#include "cmCPackLog.h"

#include <cmsys/SystemTools.hxx>

//----------------------------------------------------------------------
cmCPackCygwinBinaryGenerator::cmCPackCygwinBinaryGenerator()
{
  this->Compress = false;
}

//----------------------------------------------------------------------
cmCPackCygwinBinaryGenerator::~cmCPackCygwinBinaryGenerator()
{
}

//----------------------------------------------------------------------
int cmCPackCygwinBinaryGenerator::InitializeInternal()
{
  this->SetOptionIfNotSet("CPACK_INCLUDE_TOPLEVEL_DIRECTORY", "1");
  std::vector<std::string> path;
  std::string pkgPath = cmSystemTools::FindProgram("bzip2", path, false);
  if ( pkgPath.empty() )
    {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot find BZip2" << std::endl);
    return 0;
    }
  this->SetOptionIfNotSet("CPACK_INSTALLER_PROGRAM", pkgPath.c_str());
  cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Found Compress program: "
    << pkgPath.c_str()
    << std::endl);

  return this->Superclass::InitializeInternal();
}

//----------------------------------------------------------------------
int cmCPackCygwinBinaryGenerator::CompressFiles(const char* outFileName,
  const char* toplevel, const std::vector<std::string>& files)
{
  std::string packageName = this->GetOption("CPACK_PACKAGE_NAME");
  packageName += this->GetOption("CPACK_PACKAGE_VERSION");
  packageName = cmsys::SystemTools::LowerCase(packageName);
  std::string manifest = "/share/doc/";
  manifest += packageName;
  manifest += "/MANIFEST";
  std::string manifestFile 
    = this->GetOption("CPACK_TEMPORARY_DIRECTORY");
  std::string tempdir = manifestFile;
  manifestFile += manifest;
  // create an extra scope to force the stream
  // to create the file before the super class is called
  {
  cmGeneratedFileStream ofs(manifestFile.c_str());
  for(std::vector<std::string>::const_iterator i = files.begin();
      i != files.end(); ++i)
    {
#undef cerr
    // remove the temp dir and replace with /usr
    ofs << "/usr" << (*i).substr(tempdir.size()) << "\n";
    std::cerr << "/usr" << (*i).substr(tempdir.size()) << "\n";
    
    }
  ofs << "/usr" << manifest << "\n";
  }
  // Now compress up everything
  std::vector<std::string> filesWithManifest = files;
  filesWithManifest.push_back(manifestFile);
  return this->Superclass::CompressFiles(outFileName, toplevel, 
                                         filesWithManifest);
}

