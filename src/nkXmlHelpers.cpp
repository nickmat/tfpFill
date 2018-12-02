/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        nkRefDocuments.cpp
 * Project:     tfpnick: Private utility to create Matthews TFP database
 * Purpose:     Read and process Reference Document files
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     23rd September 2011
 * Copyright:   Copyright (c) 2011, Nick Matthews.
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

#include <wx/sstream.h>

#include "nkMain.h"
#include "xml2.h"

wxString xmlGetSource( wxXmlNode* node )
{
    if( !node ) return wxEmptyString;

    wxString str;
    wxStringOutputStream output( &str );
    wxXmlDocument doc;
    wxXmlNode* root = new wxXmlNode( NULL, node->GetType(), node->GetName() );
    root->SetAttributes( node->GetAttributes() );
    root->SetChildren( node->GetChildren() );
    doc.SetRoot( root );
    doc.Save( output, 0, wxXMLDOC_SAVE_NONE );
    root->SetAttributes( NULL );
    root->SetChildren( NULL );
    return str;
}

wxString xmlGetAllContent( wxXmlNode* node )
{
    if( !node ) return wxEmptyString;

    wxString str;
    wxXmlNode* child = node->GetChildren();
    while( child ) {
        switch( child->GetType() )
        {
        case wxXML_TEXT_NODE:
        case wxXML_CDATA_SECTION_NODE :
            str << child->GetContent();
            break;
        case wxXML_ELEMENT_NODE:
            if( child->GetName() == "br" ) {
                str << " ";
                break;
            }
            str << xmlGetAllContent( child );
            break;
        }
        child = child->GetNext();
    }
    return str;
}

wxXmlNode* xmlGetFirstText( wxXmlNode* node )
{
    if( !node ) return NULL;

    wxXmlNode* child = node->GetChildren();
    while( child ) {
        switch( child->GetType() )
        {
        case wxXML_TEXT_NODE:
            return child;
        case wxXML_ELEMENT_NODE:
            return xmlGetFirstText( child );
        }
        child = child->GetNext();
    }
    return NULL;
}

wxXmlNode* xmlGetFirst( wxXmlNode* node, const wxString& tag )
{
    while( node && node->GetName() != tag ) {
        node = node->GetNext();
    }
    return node;
}

wxXmlNode* xmlGetNext( wxXmlNode* node, const wxString& tag )
{
    if( node ) {
        node = node->GetNext();
    }
    return xmlGetFirst( node, tag );
}

wxXmlNode* xmlGetChild( wxXmlNode* node )
{
    if( node ) {
        node = node->GetChildren();
    }
    return node;
}

wxXmlNode* xmlGetFirstChild( wxXmlNode* node, const wxString& tag )
{
    return xmlGetFirst( xmlGetChild( node ), tag );
}

wxXmlNode* xmlGetFirstTag( wxXmlNode* node, const wxString & tag )
{
    if ( node->GetType() == wxXML_ELEMENT_NODE && node->GetName() == tag ) {
        return node;
    }
    return xmlGetNextTag( node, tag );
}

wxXmlNode* xmlGetNextTag( wxXmlNode* node, const wxString& tag )
{
    wxXmlNode* next = node->GetChildren();
    next = xmlGetFirstTag( next, tag );
    if ( next ) {
        return next;
    }
    for ( ;; ) {
        next = node->GetNext();
        if ( next == 0 ) {
            break;
        }
        next = xmlGetFirstTag( next, tag );
        if ( next ) {
            return next;
        }
    }
    



    while ( node ) {
        if ( node->GetType() == wxXML_ELEMENT_NODE ) {
            if ( node->GetName() == tag ) {
                return node;
            } else {
                return xmlGetNextTag( node->GetChildren(), tag );
            }
        }
        node = node->GetNext();
    }
    return 0;
}

idt GetIndividualAnchor( wxXmlNode* node, wxString* name, wxXmlNode** aNode )
{
    while ( node ) {
        if ( node->GetType() == wxXML_ELEMENT_NODE ) {
            if ( node->GetName() == "a" ) {
                *aNode = node;
                wxString href = node->GetAttribute( "href" );
                idt id;
                if ( DecodeHref( href, &id, NULL ) ) {
                    *name = xmlGetAllContent( node );
                }
                return id;
            } else {
                return GetIndividualAnchor( node->GetChildren(), name, aNode );
            }
        }
        node = node->GetNext();
    }
    return 0;
}

wxString xmlReadRecLoc( const wxString& str )
{
    wxString input, address;
    input 
        << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
           "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
           "<head>\n<title>Title</title>\n</head>\n<body>\n" << str << "\n</body>\n</html>\n";

    wxXmlDocument doc;
    wxStringInputStream sis( input );
    if( doc.Load( sis ) ) {
        wxXmlNode* node = doc.GetRoot();
        node = xmlGetFirst( node->GetChildren(), "body" );
        node = xmlGetFirst( node->GetChildren(), "table" );
        node = xmlGetFirst( node->GetChildren(), "tr" );
        node = xmlGetFirst( node->GetChildren(), "td" );
        node = xmlGetNext( node, "td" );
        if( xmlGetAllContent( node ) == "Address:" ) {
            node = xmlGetNext( node, "td" );
            address = xmlGetAllContent( node );
        }
    }

    return address;
}

wxXmlNode* xmlCreateLink( wxXmlNode* node, const wxString& href )
{
    if( !node ) return NULL;
    wxASSERT( node->GetType() == wxXML_ELEMENT_NODE );
    wxXmlNode* link = new wxXmlNode( wxXML_ELEMENT_NODE, "a" );
    link->AddAttribute( "href", href );
    link->SetParent( node );
    link->SetChildren( node->GetChildren() );
    for( wxXmlNode* n = node->GetChildren() ; n ; n = n->GetNext() ) {
        n->SetParent( link );
    }
    node->SetChildren( link );
    return link;
}

wxXmlNode* xmlCreateLink( wxXmlNode* node, int beg, int end, const wxString& href )
{
    if( node->GetType() != wxXML_TEXT_NODE ) {
        node = xmlGetFirstText( node );
    }
    if( !node ) return NULL;

    wxString str = node->GetContent();
    if( end == -1 || end > (int) str.length() ) {
        end = str.length();
    } 
    if( beg >= end ) return NULL;
    wxString begStr = str.Mid( 0, beg );
    wxString midStr = str.Mid( beg, end );
    wxString endStr = str.Mid( end );

    wxXmlNode* parent = node->GetParent();
    wxXmlNode* elderSibling = NULL;
    for( wxXmlNode* n = parent->GetChildren() ; n != node ; n = n->GetNext() ) {
        wxASSERT( n != NULL );
        elderSibling = n;
    }
    wxXmlNode* youngerSibling = node->GetNext();

    wxXmlNode* link = new wxXmlNode( wxXML_ELEMENT_NODE, "a" );
    link->AddAttribute( "href", href );
    link->SetParent( parent );
    link->SetChildren( node );
    node->SetContent( midStr );
    node->SetParent( link );
    node->SetNext( NULL );

    if( begStr.size() ) {
        wxXmlNode* begNode = new wxXmlNode( wxXML_TEXT_NODE, "text", begStr );
        begNode->SetParent( parent );
        if( elderSibling == NULL ) {
            parent->SetChildren( begNode );
        } else {
            elderSibling->SetNext( begNode );
        }
        elderSibling = begNode;
    }

    if( elderSibling == NULL ) {
        parent->SetChildren( link );
    } else {
        elderSibling->SetNext( link );
    }

    if( endStr.size() ) {
        wxXmlNode* endNode = new wxXmlNode( wxXML_TEXT_NODE, "text", endStr );
        endNode->SetParent( parent );
        link->SetNext( endNode );
        endNode->SetNext( youngerSibling );
    } else {
        link->SetNext( youngerSibling );
    }
    return link;
}

bool xmlChangeLink( wxXmlNode* node, const wxString& href )
{
    if ( node->GetName() != "a" ) {
        return false;
    }
    for ( wxXmlAttribute* attr = node->GetAttributes(); attr; attr->GetNext() ) {
        if ( attr->GetName() == "href" ) {
            attr->SetValue( href );
            return true;
        }
    }
    return false;
}

wxString xmlCreateHref( recEntity entity, idt id )
{
    wxString href;
    if ( id == 0 ) {
        return href;
    }
    switch ( entity )
    {
    case recENT_Persona:
        href = "tfpr:Pa";
        break;
    case recENT_Eventa:
        href = "tfpr:Ea";
        break;
    case recENT_Name:
        href = "tfpi:N";
        break;
    case recENT_Date:
        href = "tfpi:D";
        break;
    case recENT_Place:
        href = "tfpi:P";
        break;
    default:
        return href;
    }
    return href + recGetStr( id );
}

wxXmlNode* xmlCreateLink( wxXmlNode* node, recEntity entity, idt id )
{
    wxString href = xmlCreateHref( entity, id );
    if ( href.empty() ) {
        return node;
    }
    return xmlCreateLink( node, href );
}

wxXmlNode* xmlCreateLink( wxXmlNode* node, int beg, int end, recEntity entity, idt id )
{
    wxString href = xmlCreateHref( entity, id );
    if ( href.empty() ) {
        return node;
    }
    return xmlCreateLink( node, beg, end, href );
}

bool xmlChangeLink( wxXmlNode* node, recEntity entity, idt id )
{
    wxString href = xmlCreateHref( entity, id );
    if ( href.empty() ) {
        return false;
    }
    return xmlChangeLink( node, href );
}

// End of nkRefDocuments.cpp file 
