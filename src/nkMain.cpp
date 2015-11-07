/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        tfp_fill/src/nkMain.cpp
 * Project:     tfp_fill: Private utility to create Matthews TFP database
 * Purpose:     Application main and supporting functions
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     23rd September 2011
 * Copyright:   Copyright (c) 2011 ~ 2015, Nick Matthews.
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

#include <wx/cmdline.h>
#include <wx/tokenzr.h>
#include <wx/textfile.h>
#include <wx/filename.h>
#include <wx/wfstream.h>

#include <rec/recDb.h>

#include "nkMain.h"
#include "xml2.h"

#define VERSION   "0.1.0"
#define PROGNAME  "fill for TFP"
#define PROGDATE  "2011 - 2015"

const wxString g_version = VERSION;
const wxString g_progName = PROGNAME;
#ifdef _DEBUG
#define VERSION_CONFIG   " debug"
#else
#define VERSION_CONFIG   ""
#endif

const wxString g_title = PROGNAME " - Version " VERSION VERSION_CONFIG "\n"
                         "Copyright (c) " PROGDATE " Nick Matthews\n\n";

bool g_verbose = false;
bool g_quiet   = false;

bool UpdateFamilyLink( idt indID, idt famID )
{
    recIndividual ind(indID);
    recFamily fam(famID);

    if( ind.f_id == 0 ) {
        // The individual should exist
        return false;
    }
    fam.f_id = famID;
    if( ind.f_sex == SEX_Female ) {
        fam.f_wife_id = indID;
    } else {
        fam.f_husb_id = indID;
    }
    fam.Save();
    ind.f_fam_id = famID;
    ind.Save();

    return true;
}

bool UpdateFamilyLinks( const wxString& indFName )
{
    wxStringTokenizer tk;
    wxString sName;
    wxString gName;
    wxString dateStr;
    wxString indIdStr;
    wxString famIdStr;
    idt indID, famID;

    wxTextFile iFile;
    iFile.Open( indFName );
    wxString line;
    int state = 0;
    for( line = iFile.GetFirstLine() ; !iFile.Eof() ; line = iFile.GetNextLine() ) {
        switch( state )
        {
        case 0:
            if( line == wxEmptyString ) state = 1;
            break;
        case 1:
            if( line == wxEmptyString ) {
                state = 2;
                break;
            }
            tk.SetString( line, ",\n" );
            sName = tk.GetNextToken();
            gName = tk.GetNextToken();
            dateStr = tk.GetNextToken();
            indIdStr = tk.GetNextToken();
            famIdStr = tk.GetNextToken();
            indIdStr.ToLongLong( &indID );
            famIdStr.ToLongLong( &famID );
            if( !UpdateFamilyLink( indID, famID ) ) return false;
        }
    }
    return true;
}

void TweakDatabase()
{
    recIndividual ind;

    ind.ReadID( 3 ); // Chris
    ind.FSetPrivacy( 20 ); // Set privacy to 20 (Name only)
    ind.Save();
    ind.ReadID( 4 ); // Juliette
    ind.FSetPrivacy( 20 );
    ind.Save();
    ind.ReadID( 5 ); // Nick
    ind.FSetPrivacy( 20 );
    ind.Save();
}

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
        { wxCMD_LINE_SWITCH, "t", "test",    "do test and quit" },
        { wxCMD_LINE_PARAM,  NULL, NULL, "command-file",
            wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY },
        { wxCMD_LINE_NONE }
    };
    int ret = EXIT_FAILURE;

    wxInitializer initializer;
    if( !initializer )
    {
        fprintf( stderr, "Failed to initialize the wxWidgets library, aborting." );
        return EXIT_FAILURE;
    }

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


    if( parser.Found( "t" ) ) {
        return DoTest( parser.GetParam() );
    }

    wxFileName inFName( parser.GetParam() );
    if( !inFName.FileExists() ) {
        wxPrintf( "Input file \"%s\" not found.\n", inFName.GetFullPath() );
        return EXIT_FAILURE;
    }

    recInitialize();
    wxPrintf( "SQLite3 version: %s\n", wxSQLite3Database::GetVersion() );

    wxString famIdxFile;
    wxString gedcomFile;
    wxString refFolder;
    wxString refFile;
    wxString outFile;
    wxTextFile cFile;
    cFile.Open( inFName.GetFullPath() );
    wxString line;
    for( line = cFile.GetFirstLine() ; !cFile.Eof() ; line = cFile.GetNextLine() ) {
        if( line.compare( 0, 17, "input-gedcom-file" ) == 0 ) {
            gedcomFile = line.Mid( 18 );
            continue;
        }
        if( line.compare( 0, 18, "input-family-index" ) == 0 ) {
            famIdxFile = line.Mid( 19 );
            continue;
        }
        if( line.compare( 0, 16, "input-ref-folder" ) == 0 ) {
            refFolder = line.Mid( 17 );
            continue;
        }
        if( line.compare( 0, 14, "input-ref-file" ) == 0 ) {
            refFile = line.Mid( 15 );
            continue;
        }
        if( line.compare( 0, 11, "output-file" ) == 0 ) {
            outFile = line.Mid( 12 );
            continue;
        }
    }
    cFile.Close();
    wxPrintf( "Current: %s\n", wxGetCwd() );
    wxPrintf( "Command File: [%s]\n", inFName.GetFullPath() );
    wxPrintf( "GEDCOM file: [%s]\n", gedcomFile );
    wxPrintf( "Family index file: [%s]\n", famIdxFile );
    wxPrintf( "Reference folder: [%s]\n", refFolder );
    wxPrintf( "Reference file: [%s]\n", refFile );
    wxPrintf( "Output file: [%s]\n", outFile );

    if( wxFileExists( outFile ) ) {
        wxRemoveFile( outFile );
    }
    wxPrintf( "Creating database" );
    if( recDb::CreateDb( outFile, 0 ) ) {
        wxPrintf( " Done\nImporting GEDCOM " );
        recGedParse ged( gedcomFile );
        ged.SetUseXref( true );
        wxPrintf( "." );
        unsigned flags = recGED_IMPORT_NO_POST_OPS | recGED_IMPORT_NO_SOUR_REC;
        if( ged.Import( flags ) ) {
            wxPrintf( " Done.\nUpdating links " );
            recDb::Begin();
            UpdateFamilyLinks( famIdxFile );
            wxPrintf( " Done.\nInput Ref Breakdown File " );
            InputRefBreakdownFile( refFile );
            wxPrintf( " Done.\nInput Ref Doc Files " );
            InputRefFiles( refFolder );

            wxPrintf( " Done.\nTweak database " );
            TweakDatabase();

            recDb::Commit();
            wxPrintf( " Done.\nCleaning up Database " );
            ret = EXIT_SUCCESS;
        } else {
            wxPrintf( " Failed.\n" );
        }
        wxPrintf( " Done.\nCreated %s Database file.\n\n", recDb::GetFileName() );
    } else {
        wxPrintf( " Failed.\n" );
    }
    recUninitialize();
    return ret;
}

bool DecodeHref( const wxString& href, idt* indID, wxString* indIdStr )
{
    long dir, fnum, id; 
    // Old Individual href format = "../ps01/ps01_016.htm"
    // TFP format is "tfp:I16"
    if( href.Mid( 0, 5 ) == "tfp:I" ) { 
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

wxString GetLastWord( const wxString& address )
{
    return address.AfterLast( ' ' );
}


// End of nkMain.cpp file 
