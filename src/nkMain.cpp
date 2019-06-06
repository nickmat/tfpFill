/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        tfp_fill/src/nkMain.cpp
 * Project:     tfp_fill: Private utility to create Matthews TFP database
 * Purpose:     Application main and supporting functions
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     23rd September 2011
 * Copyright:   Copyright (c) 2011 ~ 2019, Nick Matthews.
 * Licence:     GNU GPLv3
 *
 *  tfpnick is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  tfpnick is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with tfpnick.  If not, see <http://www.gnu.org/licenses/>.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

*/

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "nkMain.h"
#include "xml2.h"

#include <rec/recDb.h>

#include <wx/cmdline.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>

#include <ctime>


#define VERSION   "0.3.1"
#define PROGNAME  "fill for TFP"
#define PROGDATE  "2011 ~ 2018"

const wxString g_version = VERSION;
const wxString g_progName = PROGNAME;
#ifdef _DEBUG
#define VERSION_CONFIG   " debug"
#else
#define VERSION_CONFIG   ""
#endif

const wxString g_title = PROGNAME " - Version " VERSION VERSION_CONFIG "\n"
                         "Copyright (c) " PROGDATE " Nick Matthews\n\n";

/* Revision history

  8jan17  V0.2.0 - Includes all code for creating database from GEDCOM plus others.
 24dec17  V0.3.0 - Based on adding data from rd*.htm files only.
  active  V0.3.1 - Now adds media (image) files as well.
*/

bool g_verbose = false;
bool g_quiet   = false;

/*#*************************************************************************
 **  main
 **  ~~~~
 */

int main( int argc, char** argv )
{
    static const wxCmdLineEntryDesc desc[] = {
        { wxCMD_LINE_SWITCH, "h", "help", "show this help message",
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
        { wxCMD_LINE_SWITCH, "v", "verbose", "be verbose" },
        { wxCMD_LINE_SWITCH, "q", "quiet",   "be quiet" },
        { wxCMD_LINE_PARAM,  NULL, NULL, "command-file",
            wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY },
        { wxCMD_LINE_NONE }
    };
    clock_t ticks = clock();
    int ret = EXIT_FAILURE;

    wxInitializer initializer;
    if( !initializer )
    {
        fprintf( stderr, "Failed to initialize the wxWidgets library, aborting." );
        return EXIT_FAILURE;
    }
    recInitialize();

    wxCmdLineParser parser( desc, argc, argv );
    parser.SetLogo( g_title );

    int cmd = parser.Parse();
    if( cmd < 0 ) {
        return EXIT_SUCCESS;
    }
    if( cmd > 0 ) {
        return EXIT_FAILURE;
    }
    if( parser.Found( "q" ) ) g_quiet = true;
    if( parser.Found( "v" ) ) g_verbose = true;

    if( ! g_quiet ) wxPrintf( g_title );

    wxFileName configName( parser.GetParam() );
    configName.MakeAbsolute();

    if( !configName.FileExists() ) {
        wxPrintf( "Input file \"%s\" not found.\n", configName.GetFullPath() );
        return EXIT_FAILURE;
    }

    wxFileConfig conf( "", "", configName.GetFullPath(), "", wxCONFIG_USE_LOCAL_FILE );

    wxString initDatabase = conf.Read( "/Input/Initial-Database" );
    wxString refFolder = conf.Read( "/Input/Ref-Folder" );
    wxString imgFolder = conf.Read( "/Input/Image-Folder" );
    wxString outFile = conf.Read( "/Output/Database" );
    wxString outMediaFile = conf.Read( "/Output/Media" );

    wxPrintf( "Database version: %s\n", recVerStr );
    wxPrintf( "SQLite3 version: %s\n", wxSQLite3Database::GetVersion() );
    wxPrintf( "Current folder: [%s]\n", wxGetCwd() );
    wxPrintf( "Configuration File: [%s]\n\n", configName.GetFullPath() );
    wxPrintf( "Initial Database: [%s]\n", initDatabase );
    wxPrintf( "Reference folder: [%s]\n", refFolder );
    wxPrintf( "Image folder: [%s]\n", imgFolder );
    wxPrintf( "Output database file: [%s]\n", outFile );
    wxPrintf( "Media database file: [%s]\n", outMediaFile );

    if( wxFileExists( outFile ) ) {
        wxRemoveFile( outFile );
    }
    if( wxFileExists( initDatabase ) ) {
        wxPrintf( "\nCopying intitial database" );
        wxCopyFile( initDatabase, outFile );
        if( recDb::OpenDb( outFile ) != recDb::DT_Full ) {
            wxPrintf( "\nCan't open Database.\n" );
            recUninitialize();
            return EXIT_FAILURE;
        }
    } else {
        wxPrintf( "\nCreating database" );
        if( recDb::CreateDbFile( outFile, recDb::DT_Full ) != recDb::CR_OK ) {
            wxPrintf( "\nCan't create Database.\n" );
            recUninitialize();
            return EXIT_FAILURE;
        }
    }

    if ( wxFileExists( outMediaFile ) ) {
        wxRemoveFile( outMediaFile );
    }
    if ( !outMediaFile.empty() ) {
        wxPrintf( "\nCreating media database" );
        if ( recDb::CreateDbFile( outMediaFile, recDb::DT_MediaOnly ) != recDb::CR_OK ) {
            wxPrintf( "\nCan't Create Media Database.\n" );
            recUninitialize();
            return EXIT_FAILURE;
        }
        if ( !recDb::AttachDb( outMediaFile, "Media" ) ) {
            wxPrintf( "\nCan't Attach Media Database.\n" );
            recUninitialize();
            return EXIT_FAILURE;
        }
    }

    recDb::Begin();
#if 1
    MediaVec media;
    if ( !refFolder.empty() ) {
        wxPrintf( " Done.\nInput Ref Doc Files " );
        InputRefFiles( refFolder, media );
    }
    if ( !imgFolder.empty() ) {
        wxPrintf( " Done.\nInput Image Files " );
        InputMediaFiles( imgFolder );
    }
    if ( !outMediaFile.empty() ) {
        wxPrintf( " Done.\nCreate Media Database " );
        OutputMediaDatabase( outMediaFile, refFolder, media );
    }

#else
    Filenames customs;
    ProcessRefFile( "../web/rd01/rd00300.htm", 300, customs );
#endif

    recDb::Commit();

    ret = EXIT_SUCCESS;
    wxPrintf( " Done.\n" );

    wxPrintf( "\nCreated %s Database file.\n", recDb::GetFileName() );
    int s = (clock() - ticks) / CLOCKS_PER_SEC;
    int m = (int) s / 60;
    std::cout << "Timed: " << m << "m " << s - (m*60) << "s\n\n";

    recUninitialize();
    return ret;
}

recEntity DecodeOldHref( const wxString & href )
{
    recEntity ent = recENT_NULL;
    wxString start = href.Mid( 0, 5 );
    if ( start == "../ps" ) {
        ent = recENT_Individual;
    } else if ( start == "../wc" ) {
        ent = recENT_Family;
    }
    return ent;
}

bool DecodeHref( const wxString& href, idt* indID, wxString* indIdStr )
{
    long dir, fnum, id; 
    // Old Individual href format = "../ps01/ps01_016.htm"
    // TFP format is "tfp:I16"
    if( href.Mid( 0, 4 ) == "tfp:" ) { 
        // Already converted.
        if( !href.Mid( 5 ).ToLong( &id ) ) return false;
    } else {
        if( href.Mid( 0, 5 ) != "../ps" ) return false;
        if( !href.Mid( 5, 2 ).ToLong( &dir ) ) return false;
        if( !href.Mid( 13, 3 ).ToLong( &fnum ) ) return false;
        id = (dir-1)*500 + fnum;
        if( id < 1 ) return false;
    }
    *indID = id;
    if( indIdStr ) {
        *indIdStr = wxString::Format( "tfp:I%ld", id );
    }
    return true;
}

wxString CreateCommaList( wxString& first, wxString& second )
{
    first.Trim();
    second.Trim();
    if( first == wxEmptyString ) return second;
    if( second == wxEmptyString ) return first;
    return first + ", " + second;
}

// End of nkMain.cpp file 
