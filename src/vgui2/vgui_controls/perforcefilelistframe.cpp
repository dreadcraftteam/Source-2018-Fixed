//========= Copyright Valve Corporation, All rights reserved. ============//
//
// List of perforce files and operations
//
//=============================================================================

#include "vgui_controls/perforcefilelistframe.h"
#include "tier1/KeyValues.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/ListPanel.h"
#include "vgui_controls/Splitter.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/MessageBox.h"
#include "tier2/tier2.h"
#include "filesystem.h"
#include "vgui/IVGui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Sort by asset name
//-----------------------------------------------------------------------------
static int __cdecl OperationSortFunc( vgui::ListPanel *pPanel, const vgui::ListPanelItem &item1, const vgui::ListPanelItem &item2 )
{
	const char *string1 = item1.kv->GetString("operation");
	const char *string2 = item2.kv->GetString("operation");
	int nRetVal = Q_stricmp( string1, string2 );
	if ( nRetVal != 0 )
		return nRetVal;

	string1 = item1.kv->GetString("filename");
	string2 = item2.kv->GetString("filename");
	return Q_stricmp( string1, string2 );
}

static int __cdecl FileBrowserSortFunc( vgui::ListPanel *pPanel, const vgui::ListPanelItem &item1, const vgui::ListPanelItem &item2 )
{
	const char *string1 = item1.kv->GetString("filename");
	const char *string2 = item2.kv->GetString("filename");
	return Q_stricmp( string1, string2 );
}

			 
//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
COperationFileListFrame::COperationFileListFrame( vgui::Panel *pParent, const char *pTitle, const char *pColumnHeader, bool bShowDescription, bool bShowOkOnly, int nDialogID ) :
	BaseClass( pParent, "PerforceFileList" )
{
	m_pText = NULL;
	vgui::Panel *pBrowserParent = this;

	m_pDescription = NULL;
	m_pSplitter = NULL;
	if ( bShowDescription )
	{
		m_pSplitter = new vgui::Splitter( this, "Splitter", vgui::SPLITTER_MODE_HORIZONTAL, 1 );

		pBrowserParent = m_pSplitter->GetChild( 0 );
		vgui::Panel *pDescParent = m_pSplitter->GetChild( 1 );

		m_pDescription = new vgui::TextEntry( pDescParent, "Description" );
		m_pDescription->SetMultiline( true );
		m_pDescription->SetCatchEnterKey( true );
		m_pDescription->SetText( "<enter description here>" );
	}

	// FIXME: Might be nice to have checkboxes per row
	m_pFileBrowser = new vgui::ListPanel( pBrowserParent, "Browser" );
 	m_pFileBrowser->AddColumnHeader( 0, "operation", "Operation", 52, 0 );
	m_pFileBrowser->AddColumnHeader( 1, "filename", pColumnHeader, 128, vgui::ListPanel::COLUMN_RESIZEWITHWINDOW );
    m_pFileBrowser->SetSelectIndividualCells( false );
	m_pFileBrowser->SetMultiselectEnabled( false );
	m_pFileBrowser->SetEmptyListText( "No Perforce Operations" );
 	m_pFileBrowser->SetDragEnabled( true );
 	m_pFileBrowser->AddActionSignalTarget( this );
	m_pFileBrowser->SetSortFunc( 0, OperationSortFunc );
	m_pFileBrowser->SetSortFunc( 1, FileBrowserSortFunc );
	m_pFileBrowser->SetSortColumn( 0 );

	m_pYesButton = new vgui::Button( this, "YesButton", "Yes", this, "Yes" );
	m_pNoButton = new vgui::Button( this, "NoButton", "No", this, "No" );

	SetBlockDragChaining( true );
	SetDeleteSelfOnClose( true );

	if ( bShowDescription )
	{
		LoadControlSettingsAndUserConfig( "resource/perforcefilelistdescription.res", nDialogID );
	}
	else
	{
		LoadControlSettingsAndUserConfig( "resource/perforcefilelist.res", nDialogID );
	}

	if ( bShowOkOnly )
	{
		m_pYesButton->SetText( "#MessageBox_OK" );
		m_pNoButton->SetVisible( false );
	}

	m_pContextKeyValues = NULL;

	SetTitle( pTitle, false );
}

COperationFileListFrame::~COperationFileListFrame()
{
	SaveUserConfig();
	CleanUpMessage();
	if ( m_pText )
	{
		delete[] m_pText;
	}
}


//-----------------------------------------------------------------------------
// Deletes the message
//-----------------------------------------------------------------------------
void COperationFileListFrame::CleanUpMessage()
{
	if ( m_pContextKeyValues )
	{
		m_pContextKeyValues->deleteThis();
		m_pContextKeyValues = NULL;
	}
}


//-----------------------------------------------------------------------------
// Performs layout
//-----------------------------------------------------------------------------
void COperationFileListFrame::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( m_pSplitter )
	{
		int x, y, w, h;
		GetClientArea( x, y, w, h );
		y += 6;
		h -= 36;
		m_pSplitter->SetBounds( x, y, w, h );
	}
}


//-----------------------------------------------------------------------------
// Adds files to the frame
//-----------------------------------------------------------------------------
void COperationFileListFrame::ClearAllOperations()
{
	m_pFileBrowser->RemoveAll();	
}


//-----------------------------------------------------------------------------
// Adds the strings to the list panel
//-----------------------------------------------------------------------------
void COperationFileListFrame::AddOperation( const char *pOperation, const char *pFileName )
{
	KeyValues *kv = new KeyValues( "node", "filename", pFileName );
	kv->SetString( "operation", pOperation );
	m_pFileBrowser->AddItem( kv, 0, false, false );
}

void COperationFileListFrame::AddOperation( const char *pOperation, const char *pFileName, const Color& clr )
{
	KeyValues *kv = new KeyValues( "node", "filename", pFileName );
	kv->SetString( "operation", pOperation );
	kv->SetColor( "cellcolor", clr );
	m_pFileBrowser->AddItem( kv, 0, false, false );
}


//-----------------------------------------------------------------------------
// Resizes the operation column to fit the operation text
//-----------------------------------------------------------------------------
void COperationFileListFrame::ResizeOperationColumnToContents()
{
	m_pFileBrowser->ResizeColumnToContents( 0 );
}


//-----------------------------------------------------------------------------
// Sets the column header for the 'operation' column
//-----------------------------------------------------------------------------
void COperationFileListFrame::SetOperationColumnHeaderText( const char *pText )
{
	m_pFileBrowser->SetColumnHeaderText( 0, pText );
}


//-----------------------------------------------------------------------------
// Adds the strings to the list panel
//-----------------------------------------------------------------------------
void COperationFileListFrame::DoModal( KeyValues *pContextKeyValues, const char *pMessage )
{
	m_MessageName = pMessage ? pMessage : "OperationConfirmed";
	CleanUpMessage();
	m_pContextKeyValues = pContextKeyValues;
	m_pFileBrowser->SortList();
	if ( m_pNoButton->IsVisible() )
	{
		m_pYesButton->SetEnabled( m_pFileBrowser->GetItemCount() != 0 );
	}
	BaseClass::DoModal();
}


//-----------------------------------------------------------------------------
// Retrieves the number of files, the file names, and operations
//-----------------------------------------------------------------------------
int COperationFileListFrame::GetOperationCount()
{
	return m_pFileBrowser->GetItemCount();
}

const char *COperationFileListFrame::GetFileName( int i )
{
	int nItemId = m_pFileBrowser->GetItemIDFromRow( i );
	KeyValues *pKeyValues = m_pFileBrowser->GetItem( nItemId );
	return pKeyValues->GetString( "filename" );
}

const char *COperationFileListFrame::GetOperation( int i )
{
	int nItemId = m_pFileBrowser->GetItemIDFromRow( i );
	KeyValues *pKeyValues = m_pFileBrowser->GetItem( nItemId );
	return pKeyValues->GetString( "operation" );
}


//-----------------------------------------------------------------------------
// Retreives the description (only if it was shown)
//-----------------------------------------------------------------------------
const char *COperationFileListFrame::GetDescription()
{
	return m_pText; 
}

	
//-----------------------------------------------------------------------------
// Returns the message name
//-----------------------------------------------------------------------------
const char *COperationFileListFrame::CompletionMessage() 
{ 
	return m_MessageName; 
}


//-----------------------------------------------------------------------------
// On command
//-----------------------------------------------------------------------------
void COperationFileListFrame::OnCommand( const char *pCommand )
{
	if ( !Q_stricmp( pCommand, "Yes" ) )
	{
		if ( m_pDescription )
		{
			int nLen = m_pDescription->GetTextLength() + 1;
			m_pText = new char[ nLen ];
			m_pDescription->GetText( m_pText, nLen );
		}
		
		KeyValues *pActionKeys;
		if ( PerformOperation() )
		{
			pActionKeys = new KeyValues( CompletionMessage(), "operationPerformed", 1 );
		}
		else
		{
			pActionKeys = new KeyValues( CompletionMessage(), "operationPerformed", 0 );
		}

		if ( m_pContextKeyValues )
		{
			pActionKeys->AddSubKey( m_pContextKeyValues );
			m_pContextKeyValues = NULL;
		}
		CloseModal();
		PostActionSignal( pActionKeys );
		return;
	}

	if ( !Q_stricmp( pCommand, "No" ) )
	{
		KeyValues *pActionKeys = new KeyValues( CompletionMessage(), "operationPerformed", 0 );
		if ( m_pContextKeyValues )
		{
			pActionKeys->AddSubKey( m_pContextKeyValues );
			m_pContextKeyValues = NULL;
		}
		CloseModal();
		PostActionSignal( pActionKeys );
		return;
	}

	BaseClass::OnCommand( pCommand );
}


//-----------------------------------------------------------------------------
//
// Version that does the work of perforce actions
//
//-----------------------------------------------------------------------------
CPerforceFileListFrame::CPerforceFileListFrame( vgui::Panel *pParent, const char *pTitle, const char *pColumnHeader, PerforceAction_t action ) :
	BaseClass( pParent, pTitle, pColumnHeader, (action == PERFORCE_ACTION_FILE_SUBMIT), false, OPERATION_DIALOG_ID_PERFORCE ) 
{
	m_Action = action;
}

CPerforceFileListFrame::~CPerforceFileListFrame()
{
}


//-----------------------------------------------------------------------------
// Activates the modal dialog
//-----------------------------------------------------------------------------
void CPerforceFileListFrame::DoModal( KeyValues *pContextKeys, const char *pMessage )
{
	BaseClass::DoModal( pContextKeys, pMessage ? pMessage : "PerforceActionConfirmed" );
}

	
//-----------------------------------------------------------------------------
// Adds a file for open
//-----------------------------------------------------------------------------
void CPerforceFileListFrame::AddFileForOpen( const char *pFullPath )
{
};

//-----------------------------------------------------------------------------
// Add files to dialog for submit/revert dialogs
//-----------------------------------------------------------------------------
void CPerforceFileListFrame::AddFileForSubmit( const char *pFullPath, unsigned state )
{
}


//-----------------------------------------------------------------------------
// Version of AddFile that accepts full paths
//-----------------------------------------------------------------------------
void CPerforceFileListFrame::AddFile( const char *pFullPath )
{
}


//-----------------------------------------------------------------------------
// Version of AddFile that accepts relative paths + search path ids
//-----------------------------------------------------------------------------
void CPerforceFileListFrame::AddFile( const char *pRelativePath, const char *pPathId )
{
}


//-----------------------------------------------------------------------------
// Does the perforce operation
//-----------------------------------------------------------------------------
bool CPerforceFileListFrame::PerformOperation( )
{
	return true;
}


//-----------------------------------------------------------------------------
// Show the perforce query dialog
//-----------------------------------------------------------------------------
void ShowPerforceQuery( vgui::Panel *pParent, const char *pFileName, vgui::Panel *pActionSignalTarget, KeyValues *pKeyValues, PerforceAction_t actionFilter )
{
}
