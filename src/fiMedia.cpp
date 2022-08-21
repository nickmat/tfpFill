/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        src/fiMedia.cpp
 * Project:     fill: Private utility to create Matthews TFP database
 * Purpose:     Read and process ReferenceDocument and Media files
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     25th September 2018
 * Copyright:   Copyright (c) 2018 ~ 2019, Nick Matthews.
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

wxString FindTitle( const wxString& note, const wxString& imStr )
{
    size_t pos = note.find( "#Photo" );
    if ( pos == wxString::npos ) {
        return "";
    }
    pos = note.find( imStr, pos );
    if ( pos == wxString::npos ) {
        return "";
    }
    pos += imStr.size() + 1;
    size_t pos2 = note.find( '\n', pos );
    return note.substr( pos, pos2 - pos );
}

idt CreateMediaEvent( recReference& ref )
{
    idt refID = ref.FGetID();
    idt eaID = CreateMediaEventa( refID );

    wxStringInputStream statement( ref.FGetStatement() );
    wxXmlDocument doc( statement );
    wxXmlNode* node = doc.GetRoot();
    idt indID;
    int seq = 0;
    std::vector<wxXmlNode*> level;
    while( node ) {
        if( node->GetName() == "a" ) {
            wxString href = node->GetAttribute( "href" );
            if( DecodeHref( href, &indID, nullptr ) ) {
                idt namID = CreateName( xmlGetAllContent( node ) );
                recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Name, namID, &seq );
                Sex sex = recIndividual::GetSex( indID );
                idt perID = CreatePersona( refID, indID, namID, sex );
                AddPersonaToEventa( eaID, perID, recEventTypeRole::ROLE_Media_Subject );
                wxString perIdStr = wxString::Format( "tfpr:Pa" ID, perID );
                xmlChangeLink( node, perIdStr );
            }
        }
        wxXmlNode* next = node->GetChildren();
        if( next ) {
            level.push_back( node );
        }
        else {
            next = node->GetNext();
            while( !next && !level.empty() ) {
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
    return CreateEventFromEventa( eaID );
}

void CreateImage( long entry, idt galID, const wxString&  imgFolder, idt assID )
{
    wxFileName txtfilename( GetImageTextFileName( entry, imgFolder ) );
    txtfilename.MakeAbsolute();
    wxFFile txtfile( txtfilename.GetFullPath(), "r" );
    wxString text;
    txtfile.ReadAll( &text );
    assert( !text.empty() );

    size_t pos = text.find( ' ' );
    assert( pos != wxString::npos );
    pos = text.find( ' ', pos + 1 ) + 1;    // Title starts after the 1st two spaces.
    size_t pos2 = text.find( '\n' ); // And ends at the first new line.
    assert( pos != wxString::npos && pos2 != wxString::npos && pos < pos2 );
    wxString title = text.substr( pos, pos2 - pos );
    pos = pos2 + 2;
    pos2 = text.find( '\0' );
    assert( pos < pos2 );
    wxString content = text.substr( pos, pos2 - pos );

    wxFileName imgfilename( GetImageFileName( entry, imgFolder ) );
    wxMemoryBuffer imgBuff;
    wxFile infile( imgfilename.GetFullPath() );
    wxFileOffset fLen = infile.Length();
    void* tmp = imgBuff.GetAppendBuf( fLen );
    size_t iRead = infile.Read( tmp, fLen );
    imgBuff.UngetAppendBuf( iRead );

    recReference ref( 0 );
    ref.FSetHigherID( 0 );
    ref.FSetTitle( "Photo of " + title );
    ref.FSetStatement( "<!-- HTML -->\n<div class='img-text'>\n" + content + "</div>\n" );
    ref.FSetResID( 1 );
    ref.FSetUserRef( "Im" + recGetStr( entry ) );
    ref.CreateUidChanged();
    ref.Save();
    idt eveID = CreateMediaEvent( ref );

    recMediaData md( 0 );
    md.FSetTitle( title );
    md.FSetData( imgBuff );
    md.FSetType( recMediaData::Mime::image_jpeg );
    md.FSetFile( "image/" + ref.FGetUserRef() );
    md.CreateUidChanged();
    md.Save( "Photos" );

    recMedia med( 0 );
    med.FSetTitle( title );
    med.FSetDataID( md.FGetID() );
    med.FSetAssID( assID );
    med.FSetRefID( ref.FGetID() );
    med.FSetRefSeq( 1 );
    med.FSetNote( ref.FGetStatement() );
    med.CreateUidChanged();
    med.Save();

    recGalleryMedia gm( 0 );
    gm.FSetID( entry );
    gm.FSetGalID( galID );
    gm.FSetMedID( med.FGetID() );
    gm.SetNextMedSequence( galID );
    gm.Save();

    for ( auto ie : recEvent::GetIndividualEvents( eveID ) ) {
        recIndividual ind( ie.FGetIndID() );
        ie.FSetNote( FindTitle( ind.FGetNote(), ref.FGetUserRef() ) );
        ie.Save();
    }
}

void ProcessImages( idt galID, const wxString& imgFolder, wxXmlNode* node, idt assID )
{
    for ( node = node->GetChildren(); node; node = node->GetNext() ) {
        if ( node->GetName() == "entry" ) {
            wxString numStr = xmlGetAllContent( node );
            long entry;
            if ( numStr.ToLong( &entry) && entry > 0  ) {
                CreateImage( entry, galID, imgFolder, assID );
            }
        }
    }
}

void CreateGallery( const wxString& imgFolder, wxXmlNode* node, idt assID )
{
    long num = 0;
    wxString title;
    long number;
    recGallery gal(0);
    gal.CreateUidChanged();

    for ( node = node->GetChildren(); node; node = node->GetNext() ) {
        if ( node->GetName() == "number" ) {
            wxString numStr = xmlGetAllContent( node );
            if ( !numStr.ToLong( &number ) ) return;
            gal.FSetID( number );
        } else if ( node->GetName() == "title" ) {
            gal.FSetTitle( xmlGetAllContent( node ) );
        } else if ( node->GetName() == "entries" ) {
            gal.Save();
            ProcessImages( gal.FGetID(), imgFolder, node, assID );
            gal.Clear();
        }
    }
}

void ProcessGalleries( const wxString& imgFolder, wxXmlNode* node, idt assID )
{
    for ( node = node->GetChildren(); node; node = node->GetNext() ) {
        if ( node->GetName() == "gallery" ) {
            CreateGallery( imgFolder, node, assID );
        }
    }
}

bool InputMediaFiles( const wxString& imgFolder, idt assID )
{
    wxString filename = imgFolder + "/galspec.xml";
    wxFileName galfn( filename );
    wxXmlDocument galspec( galfn.GetFullPath() );
    wxXmlNode* node = galspec.GetRoot();
    assert( node != nullptr );

    for ( node = node->GetChildren(); node; node = node->GetNext() ) {
        if ( node->GetName() == "galleries" ) {
            ProcessGalleries( imgFolder, node, assID );
        }
    }
    return true;
}

bool OutputMediaDatabase( const wxString& refFolder, const MediaVec& media_vec, AssFileMap& assMap )
{
    wxString medFolder( refFolder + "/or/" );
    for ( auto media : media_vec ) {
        wxString mediadb = "Scans";
        wxFileName imgfilename( medFolder + media.filename );
        wxArrayString dirs = imgfilename.GetDirs();
        for( auto& dir : dirs ) {
            if( dir == ".." ) continue;
            if( dir == "cen" ) {
                mediadb = "Census";
            }
            if( dir == "1939uk" ){
                mediadb = "Scans";
                break;
            }
            if( dir == "usa" ) {
                break;
            }
            if( dir == "bmd" ) {
                mediadb = "BMD";
                break;
            }
        }
        wxMemoryBuffer imgBuff;
        wxFile infile( imgfilename.GetFullPath() );
        wxFileOffset fLen = infile.Length();
        void* tmp = imgBuff.GetAppendBuf( fLen );
        size_t iRead = infile.Read( tmp, fLen );
        imgBuff.UngetAppendBuf( iRead );

        wxFileName fname( "or/" + media.filename );
        wxASSERT( fname.GetExt() == "jpg" );
        fname.ClearExt();
        recMediaData md( 0 );
        md.FSetTitle( mediadb + " " + fname.GetName() );
        md.FSetData( imgBuff );
        md.FSetType( recMediaData::Mime::image_jpeg );
        md.FSetFile( fname.GetFullPath( wxPATH_UNIX ) );
        md.CreateUidChanged();
        md.Save( mediadb );
        idt mdID = md.FGetID();

        recMedia med( 0 );
        med.FSetTitle( recReference::GetTitle( media.ref ) + " " + media.text );
        med.FSetDataID( mdID );
        med.FSetAssID( assMap[mediadb] );
        med.FSetRefID( media.ref );
        med.CreateUidChanged();
        med.Save();
    }
    return true;
}

// End of src/fiMedia.cpp file
