/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        nkMain.h
 * Project:     tfpnick: Private utility to create Matthews TFP database
 * Purpose:     Application main and supporting function header.
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     23rd September 2011
 * Copyright:   Copyright (c) 2011 - 2016, Nick Matthews.
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

#ifndef NKMAIN_H
#define NKMAIN_H

#include <wx/filename.h>

#include <rec/recDatabase.h>
#include <rec/recDate.h>

class wxXmlNode;
typedef std::vector< wxFileName > Filenames;


/* nkMain.cpp */
extern bool DecodeHref( const wxString& href, idt* indID, wxString* indIdStr );
extern wxString CreateCommaList( wxString& first, wxString& second );
extern wxString GetLastWord( const wxString& address );

/* nkRefBreakdown.cpp */
extern bool InputRefBreakdownFile( const wxString& refFile );
extern void ProcessMarkupRef( idt refID, wxXmlNode* root );

/* nkRefDocuments.cpp */
extern void ProcessRefFile( const wxString path, const wxString name, Filenames& customs );
extern void ProcessRefFile2( const wxString path, const wxString name, Filenames& customs );
extern bool InputRefFiles( const wxString& refFolder );
extern void InputNoteFiles( const wxString& notesFolder );

/* nkRefDocCustom.cpp */
extern void ProcessCustomFile( wxFileName& fn );

/* nkRecHelpers.cpp */
extern bool ExportGedcom( const wxString& path );
extern idt CreateDate( const wxString& date, idt refID, int* pseq );
extern idt CreateDateFromAge( long age, idt baseID, idt refID, int* pseq );
extern idt CreatePlace( const wxString& address, idt refID, int* pseq = NULL );
extern idt CreateCensusEvent( const wxString& title, idt dID, idt pID, idt rID, int* pseq = NULL );
extern idt CreatePersona( idt refID, idt indID, const wxString& nameStr, Sex sex, int* pseq );
extern idt CreatePersona( idt refID, idt indID, idt nameID, Sex sex );
extern idt GetWifeName( const wxString& nameStr, idt husbandNameID, idt refID, int* pseq );
extern idt CreateName( const wxString& surname, const wxString& givens );
extern void AddPersonas( idt refID, recIdVec& indList, wxArrayString& names );
extern idt CreateBirthEvent( idt refID, idt perID, idt dateID, idt placeID, int* pseq );
extern idt CreateRegBirthEvent( idt refID, idt perID, idt dateID, idt placeID, int* pseq );
extern idt CreateChrisEventa( idt refID, idt perID, idt dateID, idt placeID, int* pseq );
extern idt CreateMarriageEvent( idt perID, idt dateID, idt placeID, long role, idt refID, int* pseq );
extern idt CreateDeathEventa( idt refID, idt perID, idt dateID, idt placeID, int* pseq );
extern idt CreateRegDeathEvent( idt refID, idt perID, idt dateID, idt placeID, int* pseq );
extern idt CreateRegDeathEvent( idt refID, idt perID, idt dateID, idt placeID, int* pseq );
extern idt CreateBurialEventa( idt refID, idt perID, idt dateID, idt placeID, int* pseq );
extern idt CreateResidenceEventa( idt dateID, idt placeID, idt refID, int* pseq );
extern idt CreateFamilyRelEventa( idt refID, idt perID, idt dateID, idt placeID, int* pseq );
extern void AddPersonaToEventa( idt eventID, idt perID, idt roleID );
extern idt CreateOccupation( const wxString& occ, idt refID, idt perID, idt dateID, int* pseq );
extern idt CreateCondition( const wxString& con, idt refID, idt perID, idt dateID, int* pseq );
extern idt CreateRelationship( idt per1ID, const wxString& des, idt per2ID, idt refID, int* pseq );
extern void rSetPersonaSex( idt perID, Sex sex );
extern void rAddNameToPersona( idt perID, idt nameID );
extern idt LinkOrCreateEventFromEventa( idt eaID );
extern idt CreateEventFromEventa( idt eaID );
extern bool CreateIndividual( idt indID, idt perID );

extern Sex GetSexFromStr( const wxString& str );
extern wxString GetConditionStr( Sex sex, const wxString& cond );

/* nkXmlHelpers.cpp */
class wxXmlNode;
extern wxString xmlGetSource( wxXmlNode* node );
extern wxString xmlGetAllContent( wxXmlNode* node );
extern wxXmlNode* xmlGetFirstText( wxXmlNode* node );
extern wxXmlNode* xmlGetFirst( wxXmlNode* node, const wxString& tag );
extern wxXmlNode* xmlGetNext( wxXmlNode* node, const wxString& tag );
extern wxXmlNode* xmlGetChild( wxXmlNode* node );
extern wxXmlNode* xmlGetFirstChild( wxXmlNode* node, const wxString& tag );
extern idt GetIndividualAnchor( wxXmlNode* node, wxString* name );

extern wxString xmlReadRecLoc( const wxString& str );

extern wxXmlNode* xmlCreateLink( wxXmlNode* node, const wxString& href );
extern wxXmlNode* xmlCreateLink( wxXmlNode* node, int beg, int end, const wxString& href );

/* nkTest */
extern int DoTest( const wxString& fname );

#endif // NKMAIN_H