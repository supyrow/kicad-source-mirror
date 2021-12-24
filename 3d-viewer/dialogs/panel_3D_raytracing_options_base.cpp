///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_3D_raytracing_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_3D_RAYTRACING_OPTIONS_BASE::PANEL_3D_RAYTRACING_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerRaytracingRenderOptions;
	sbSizerRaytracingRenderOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Rendering Options") ), wxVERTICAL );

	sbSizerRaytracingRenderOptions->SetMinSize( wxSize( -1,32 ) );
	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 1, 0, 20 );

	m_cbRaytracing_proceduralTextures = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Procedural textures (slow)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRaytracing_proceduralTextures->SetValue(true);
	gSizer1->Add( m_cbRaytracing_proceduralTextures, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbRaytracing_addFloor = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Add floor (slow)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRaytracing_addFloor->SetValue(true);
	gSizer1->Add( m_cbRaytracing_addFloor, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbRaytracing_antiAliasing = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Anti-aliasing (slow)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRaytracing_antiAliasing->SetValue(true);
	gSizer1->Add( m_cbRaytracing_antiAliasing, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbRaytracing_postProcessing = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Screen space ambient occlusions and global illumination reflections (slow)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRaytracing_postProcessing->SetValue(true);
	gSizer1->Add( m_cbRaytracing_postProcessing, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbSizerRaytracingRenderOptions->Add( gSizer1, 1, wxEXPAND|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizer111;
	fgSizer111 = new wxFlexGridSizer( 0, 4, 4, 8 );
	fgSizer111->SetFlexibleDirection( wxBOTH );
	fgSizer111->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer111->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText19 = new wxStaticText( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Number of Samples"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText19->Wrap( -1 );
	fgSizer111->Add( m_staticText19, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText201 = new wxStaticText( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Spread Factor %"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText201->Wrap( -1 );
	fgSizer111->Add( m_staticText201, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText211 = new wxStaticText( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Recursion Level"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText211->Wrap( -1 );
	fgSizer111->Add( m_staticText211, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_cbRaytracing_renderShadows = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Shadows:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRaytracing_renderShadows->SetValue(true);
	fgSizer111->Add( m_cbRaytracing_renderShadows, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_numSamples_Shadows = new wxSpinCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 1, 64, 0 );
	m_numSamples_Shadows->SetToolTip( _("Number of rays that will be cast, into light direction, to evaluate a shadow point") );

	fgSizer111->Add( m_numSamples_Shadows, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_spreadFactor_Shadows = new wxTextCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer111->Add( m_spreadFactor_Shadows, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	fgSizer111->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbRaytracing_showReflections = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Reflections:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRaytracing_showReflections->SetValue(true);
	fgSizer111->Add( m_cbRaytracing_showReflections, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_numSamples_Reflections = new wxSpinCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 1, 32, 0 );
	m_numSamples_Reflections->SetToolTip( _("Number of rays that will be cast to evaluate a reflection point") );

	fgSizer111->Add( m_numSamples_Reflections, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_spreadFactor_Reflections = new wxTextCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer111->Add( m_spreadFactor_Reflections, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_recursiveLevel_Reflections = new wxSpinCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 1, 5, 0 );
	m_recursiveLevel_Reflections->SetToolTip( _("Interactions number that a ray can travel through objects. (higher number of levels improve results, specially on very transparent boards)") );

	fgSizer111->Add( m_recursiveLevel_Reflections, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_cbRaytracing_showRefractions = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Refractions:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRaytracing_showRefractions->SetValue(true);
	fgSizer111->Add( m_cbRaytracing_showRefractions, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_numSamples_Refractions = new wxSpinCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 1, 5, 0 );
	m_numSamples_Refractions->SetToolTip( _("Number of rays that will be cast to evaluate a refraction point") );

	fgSizer111->Add( m_numSamples_Refractions, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_spreadFactor_Refractions = new wxTextCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer111->Add( m_spreadFactor_Refractions, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_recursiveLevel_Refractions = new wxSpinCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 1, 7, 0 );
	m_recursiveLevel_Refractions->SetToolTip( _("Number of bounces that a ray can hit reflective objects") );

	fgSizer111->Add( m_recursiveLevel_Refractions, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	sbSizerRaytracingRenderOptions->Add( fgSizer111, 0, wxTOP|wxBOTTOM|wxRIGHT|wxEXPAND, 5 );


	sbSizerRaytracingRenderOptions->Add( 0, 3, 0, wxEXPAND, 5 );


	bSizerMargins->Add( sbSizerRaytracingRenderOptions, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbSizerRaytracingLightConfiguration;
	sbSizerRaytracingLightConfiguration = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Lights Configuration") ), wxVERTICAL );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText17 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Ambient camera light:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	bSizer11->Add( m_staticText17, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_colourPickerCameraLight = new COLOR_SWATCH( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_colourPickerCameraLight->SetMinSize( wxSize( 72,20 ) );

	bSizer11->Add( m_colourPickerCameraLight, 0, wxALL, 5 );


	bSizer11->Add( 0, 0, 1, 0, 5 );

	m_staticText5 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Top light:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bSizer11->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_colourPickerTopLight = new COLOR_SWATCH( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_colourPickerTopLight->SetMinSize( wxSize( 72,20 ) );

	bSizer11->Add( m_colourPickerTopLight, 0, wxALL, 5 );


	bSizer11->Add( 0, 0, 1, 0, 5 );

	m_staticText6 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Bottom light:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	bSizer11->Add( m_staticText6, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_colourPickerBottomLight = new COLOR_SWATCH( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_colourPickerBottomLight->SetMinSize( wxSize( 72,20 ) );

	bSizer11->Add( m_colourPickerBottomLight, 0, wxALL, 5 );


	sbSizerRaytracingLightConfiguration->Add( bSizer11, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	wxFlexGridSizer* fgSizer11;
	fgSizer11 = new wxFlexGridSizer( 0, 9, 0, 0 );
	fgSizer11->AddGrowableCol( 4 );
	fgSizer11->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer11->Add( 0, 0, 0, 0, 5 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );

	m_staticText20 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Elevation (deg)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText20->Wrap( -1 );
	fgSizer11->Add( m_staticText20, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_staticText18 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Azimuth (deg)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer11->Add( m_staticText18, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );

	m_staticText27 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Elevation (deg)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText27->Wrap( -1 );
	fgSizer11->Add( m_staticText27, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );

	m_staticText28 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Azimuth (deg)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	fgSizer11->Add( m_staticText28, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );

	m_staticText21 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Light 1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	fgSizer11->Add( m_staticText21, 0, wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL, 5 );

	m_colourPickerLight1 = new COLOR_SWATCH( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_colourPickerLight1->SetMinSize( wxSize( 72,20 ) );

	fgSizer11->Add( m_colourPickerLight1, 0, wxALL, 5 );

	m_lightElevation1 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightElevation1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_lightAzimuth1 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightAzimuth1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	fgSizer11->Add( 10, 0, 0, 0, 5 );

	m_staticText22 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Light 5:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	fgSizer11->Add( m_staticText22, 0, wxALIGN_CENTER|wxLEFT, 5 );

	m_colourPickerLight5 = new COLOR_SWATCH( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_colourPickerLight5->SetMinSize( wxSize( 72,20 ) );

	fgSizer11->Add( m_colourPickerLight5, 0, wxALL, 5 );

	m_lightElevation5 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightElevation5, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_lightAzimuth5 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightAzimuth5, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticText23 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Light 2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	fgSizer11->Add( m_staticText23, 0, wxALIGN_CENTER, 5 );

	m_colourPickerLight2 = new COLOR_SWATCH( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_colourPickerLight2->SetMinSize( wxSize( 72,20 ) );

	fgSizer11->Add( m_colourPickerLight2, 0, wxALL, 5 );

	m_lightElevation2 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightElevation2, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_lightAzimuth2 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightAzimuth2, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );

	m_staticText24 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Light 6:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText24->Wrap( -1 );
	fgSizer11->Add( m_staticText24, 0, wxALIGN_CENTER|wxLEFT, 5 );

	m_colourPickerLight6 = new COLOR_SWATCH( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_colourPickerLight6->SetMinSize( wxSize( 72,20 ) );

	fgSizer11->Add( m_colourPickerLight6, 0, wxALL, 5 );

	m_lightElevation6 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightElevation6, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_lightAzimuth6 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightAzimuth6, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_staticText25 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Light 3:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText25->Wrap( -1 );
	fgSizer11->Add( m_staticText25, 0, wxALIGN_CENTER, 5 );

	m_colourPickerLight3 = new COLOR_SWATCH( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_colourPickerLight3->SetMinSize( wxSize( 72,20 ) );

	fgSizer11->Add( m_colourPickerLight3, 0, wxALL, 5 );

	m_lightElevation3 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightElevation3, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_lightAzimuth3 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightAzimuth3, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );

	m_staticText26 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Light 7:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	fgSizer11->Add( m_staticText26, 0, wxALIGN_CENTER|wxLEFT, 5 );

	m_colourPickerLight7 = new COLOR_SWATCH( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_colourPickerLight7->SetMinSize( wxSize( 72,20 ) );

	fgSizer11->Add( m_colourPickerLight7, 0, wxALL, 5 );

	m_lightElevation7 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightElevation7, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_lightAzimuth7 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightAzimuth7, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticText171 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Light 4:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText171->Wrap( -1 );
	fgSizer11->Add( m_staticText171, 0, wxALIGN_CENTER, 5 );

	m_colourPickerLight4 = new COLOR_SWATCH( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_colourPickerLight4->SetMinSize( wxSize( 72,20 ) );

	fgSizer11->Add( m_colourPickerLight4, 0, wxALL, 5 );

	m_lightElevation4 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightElevation4, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_lightAzimuth4 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightAzimuth4, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );

	m_staticText181 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Light 8:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText181->Wrap( -1 );
	fgSizer11->Add( m_staticText181, 0, wxALIGN_CENTER|wxLEFT, 5 );

	m_colourPickerLight8 = new COLOR_SWATCH( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_colourPickerLight8->SetMinSize( wxSize( 72,20 ) );

	fgSizer11->Add( m_colourPickerLight8, 0, wxALL, 5 );

	m_lightElevation8 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightElevation8, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_lightAzimuth8 = new wxTextCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_lightAzimuth8, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );


	sbSizerRaytracingLightConfiguration->Add( fgSizer11, 0, wxEXPAND|wxALL, 5 );


	bSizerMargins->Add( sbSizerRaytracingLightConfiguration, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerMargins, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
}

PANEL_3D_RAYTRACING_OPTIONS_BASE::~PANEL_3D_RAYTRACING_OPTIONS_BASE()
{
}
