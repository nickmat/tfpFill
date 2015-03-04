/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        fiRefMarkup.h
 * Project:     tfp_fill: Private utility to create Matthews TFP database
 * Purpose:     fiRefMarkup Class header.
 * Author:      Nick Matthews
 * Created:     22nd February 2015
 * Copyright:   Copyright (c) 2015, Nick Matthews.
 * Licence:     GNU GPLv3
 *
 *  tfp_fill is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  tfp_fill is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with tfp_fill.  If not, see <http://www.gnu.org/licenses/>.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

*/

#ifndef FILL_FIREFMARKUP_H
#define FILL_FIREFMARKUP_H

#include <rec/recDatabase.h>
#include <map>

class wxXmlNode;

typedef std::map< const wxString, idt > StringIdMap;

class fiRefMarkup
{
public:
    fiRefMarkup( idt refID, wxXmlNode* root ) 
        : m_referenceID(refID), m_root(root),
        m_cur_persona(0), m_cur_date(0), m_cur_place(0), m_cur_eventa(0) {}

    bool create_records();

private:
    StringVec parse_statements( const wxString& str ) const;
    wxString read_text( const wxString& input, wxString* tail ) const;
    Sex read_sex( const wxString& input, wxString* tail ) const;
    idt find_id( const wxString& recStr ) const;
    void markup_node( wxXmlNode* refNode );
    wxString convert_local_id( const wxString& localStr ) const;

    idt create_persona_rec( const wxString& str );
    idt create_name_rec( idt perID, const wxString& str );
    idt create_ind_per_rec( idt indID, idt perID );
    idt create_date_rec( const wxString& str );
    idt create_place_rec( const wxString& str );
    idt create_eventa_rec( const wxString& str );
    idt create_eventa_per_rec( const wxString& str );
    idt create_event_eventa_rec( const wxString& str );
    void create_reference_rec();

    wxXmlNode*  m_root;
    StringIdMap m_localIDs;

    idt  m_referenceID;
    idt  m_cur_persona;
    idt  m_cur_date;
    idt  m_cur_place;
    idt  m_cur_eventa;
};

#endif // FILL_FIREFMARKUP_H

// End of fiRefMarkup.cpp file 
