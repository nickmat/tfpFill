/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        src/fiMedia.cpp
 * Project:     fill: Private utility to create Matthews TFP database
 * Purpose:     Read and process ReferenceDocument and Media files
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     25th September 2018
 * Copyright:   Copyright (c) 2018, Nick Matthews.
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

#include <wx/dir.h>
#include <wx/sstream.h>
#include <wx/textfile.h>

#include <rec/recDb.h>

#include <vector>

#include "nkMain.h"
#include "xml2.h"

wxFileName GetImageFileName( long entry, const wxString& imgFolder )
{
    assert( entry < 100 );
    wxString namestr = wxString::Format( "%s/im01a/im%05ldq.jpg", imgFolder, entry );
    wxFileName name( namestr );
    name.MakeAbsolute();
    if ( name.Exists() ) {
        return name;
    }
    namestr = wxString::Format( "%s/im01a/im%05ldqc.jpg", imgFolder, entry );
    name = wxFileName( namestr );
    name.MakeAbsolute();
    if ( name.Exists() ) {
        return name;
    }
    namestr = wxString::Format( "%s/im01a/im%05ldoq.jpg", imgFolder, entry );
    name = wxFileName( namestr );
    name.MakeAbsolute();
    if ( name.Exists() ) {
        return name;
    }
    namestr = wxString::Format( "%s/im01a/im%05ldoqc.jpg", imgFolder, entry );
    name = wxFileName( namestr );
    name.MakeAbsolute();
    if ( name.Exists() ) {
        return name;
    }
    assert( false ); // Just give up!
    return wxFileName();
}

wxString GetImageTextFileName( long entry, const wxString& imgFolder )
{
    assert( entry < 100 );
    return wxString::Format( "%s/im01a/im%05ld.txt", imgFolder, entry );
}

void CreatePersonas( recReference& ref )
{
    idt refID = ref.FGetID();
    wxStringInputStream statement( ref.FGetStatement() );
    wxXmlDocument doc( statement );
    wxXmlNode* node = doc.GetRoot();
    idt indID;
    int seq = 0;
    wxString indIdStr;
    std::vector<wxXmlNode*> level;
    while(node ){
        if ( node->GetName() == "a" ) {
            wxString href = node->GetAttribute( "href" );
            if ( DecodeHref( href, &indID, &indIdStr ) ) {
                xmlChangeLink( node, indIdStr );
                idt namID = CreateName( xmlGetAllContent( node ) );
                recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Name, namID, &seq );
                Sex sex = recIndividual::GetSex( indID );
                idt perID = CreatePersona( refID, indID, namID, sex );
            }
        }
        wxXmlNode* next = node->GetChildren();
        if ( next ) {
            level.push_back( node );
        } else {
            next = node->GetNext();
            while ( !next && !level.empty() ) {
                next = level.back();
                level.pop_back();
                next = next->GetNext();
            }
        }
        node = next;
    }
    wxStringOutputStream sos;
    doc.Save( sos, wxXML_NO_INDENTATION, wxXMLDOC_SAVE_NONE );
    ref.FSetStatement( sos.GetString() );
    ref.Save();
}

void CreateImage( long entry, idt galID, const wxString&  imgFolder )
{
    wxFileName txtfilename( GetImageTextFileName( entry, imgFolder ) );
    txtfilename.MakeAbsolute();
    wxFFile txtfile( txtfilename.GetFullPath(), "r" );
    wxString text;
    txtfile.ReadAll( &text );
    assert( !text.empty() );

    size_t pos = text.find( ' ' );
    assert( pos != wxString::npos );
    pos = text.find( ' ', pos + 1 );    // Title starts after the 1st two spaces.
    size_t pos2 = text.find( '\n' ); // And ends at the first new line.
    assert( pos != wxString::npos && pos2 != wxString::npos && pos < pos2 );
    wxString title = text.substr( pos, pos2 - pos );
    pos = text.find( '\0' );
    wxString content = text.substr( pos2 + 2, pos - pos2 - 2 );

    wxFileName imgfilename( GetImageFileName( entry, imgFolder ) );
    wxMemoryBuffer imgBuff;
    wxFile infile( imgfilename.GetFullPath() );
    wxFileOffset fLen = infile.Length();
    void* tmp = imgBuff.GetAppendBuf( fLen );
    size_t iRead = infile.Read( tmp, fLen );
    imgBuff.UngetAppendBuf( iRead );

    recReference ref( 0 );
    ref.FSetTitle( "Image of " + title );
    ref.FSetStatement( "<!-- HTML -->\n<div class='img-text'>\n" + content + "</div>\n" );
    ref.FSetUserRef( "Im" + recGetStr( entry ) );
    ref.Save();

    recMediaData md( 0 );
    md.FSetData( imgBuff );
    md.Save();

    recMedia med( 0 );
    med.FSetDataID( md.FGetID() );
    med.FSetRefID( ref.FGetID() );
    med.FSetTitle( title );
    med.Save();

    recGalleryMedia gm( 0 );
    gm.FSetID( entry );
    gm.FSetGalID( galID );
    gm.FSetMedID( med.FGetID() );
    gm.SetNextMedSequence( galID );
    gm.Save();

    CreatePersonas( ref );
}

void ProcessImages( idt galID, const wxString& imgFolder, wxXmlNode* node )
{
    for ( node = node->GetChildren(); node; node = node->GetNext() ) {
        if ( node->GetName() == "entry" ) {
            wxString numStr = xmlGetAllContent( node );
            long entry;
            if ( numStr.ToLong( &entry) && entry > 0  ) {
                CreateImage( entry, galID, imgFolder);
            }
        }
    }
}

void CreateGallery( const wxString& imgFolder, wxXmlNode* node )
{
    long num = 0;
    wxString title;
    long number;
    recGallery gal(0);

    for ( node = node->GetChildren(); node; node = node->GetNext() ) {
        if ( node->GetName() == "number" ) {
            wxString numStr = xmlGetAllContent( node );
            if ( !numStr.ToLong( &number ) ) return;
            gal.FSetID( number );
        } else if ( node->GetName() == "title" ) {
            gal.FSetTitle( xmlGetAllContent( node ) );
        } else if ( node->GetName() == "entries" ) {
            gal.Save();
            ProcessImages( gal.FGetID(), imgFolder, node ); 
            gal.Clear();
        }
    }
}

void ProcessGalleries( const wxString& imgFolder, wxXmlNode* node )
{
    for ( node = node->GetChildren(); node; node = node->GetNext() ) {
        if ( node->GetName() == "gallery" ) {
            CreateGallery( imgFolder, node );
        }
    }
}

bool InputMediaFiles( const wxString& imgFolder )
{
    wxString filename = imgFolder + "/galspec.xml";
    wxFileName galfn( filename );
    wxXmlDocument galspec( galfn.GetFullPath() );
    wxXmlNode* node = galspec.GetRoot();
    assert( node != nullptr );

    for ( node = node->GetChildren(); node; node = node->GetNext() ) {
        if ( node->GetName() == "galleries" ) {
            ProcessGalleries( imgFolder, node );
        }
    }

#if 0
    wxString imdirname;

    wxDir dir( imgFolder );
    if ( !dir.Open( imgFolder ) ) {
        return false;
    }
    bool cont = dir.GetFirst( &imdirname, "im???", wxDIR_DIRS );
    while ( cont ) {
        wxPrintf( "." );
        // Process Directory
        wxDir imdir;
        wxString imfilename;
        if ( !imdir.Open( imgFolder + "/" + imdirname ) ) {
            return false;
        }
        cont = imdir.GetFirst( &imfilename, "im?????.txt", wxDIR_FILES );
        while ( cont ) {
            idt imgID = recGetID( imfilename.substr( 2 ) );
            wxString path = imgFolder + "/" + imdirname + "/" + imfilename;
            ProcessImgFile( path, imgID );
            cont = imdir.GetNext( &imfilename );
        }
        cont = dir.GetNext( &imfilename );
    }
#endif
    return true;
}

// End of src/fiMedia.cpp file
