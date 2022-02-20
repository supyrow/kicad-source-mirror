/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "board_test_utils.h"

#include <wx/filename.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <fp_shape.h>
#include <fp_text.h>
#include <pad.h>
#include <settings/settings_manager.h>
#include <pcbnew_utils/board_file_utils.h>
#include <tool/tool_manager.h>
#include <zone_filler.h>

// For the temp directory logic: can be std::filesystem in C++17
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <board_commit.h>


#define CHECK_ENUM_CLASS_EQUAL( L, R )                                                             \
    BOOST_CHECK_EQUAL( static_cast<int>( L ), static_cast<int>( R ) )


namespace KI_TEST
{

BOARD_DUMPER::BOARD_DUMPER() :
        m_dump_boards( std::getenv( "KICAD_TEST_DUMP_BOARD_FILES" ) )
{
}


void BOARD_DUMPER::DumpBoardToFile( BOARD& aBoard, const std::string& aName ) const
{
    if( !m_dump_boards )
        return;

    auto path = boost::filesystem::temp_directory_path() / aName;
    path += ".kicad_pcb";

    BOOST_TEST_MESSAGE( "Dumping board file: " << path.string() );
    ::KI_TEST::DumpBoardToFile( aBoard, path.string() );
}


void LoadBoard( SETTINGS_MANAGER& aSettingsManager, const wxString& aRelPath,
                std::unique_ptr<BOARD>& aBoard )
{
    if( aBoard )
    {
        aBoard->SetProject( nullptr );
        aBoard = nullptr;
    }

    std::string absPath = GetPcbnewTestDataDir() + aRelPath.ToStdString();
    wxFileName  projectFile( absPath + ".kicad_pro" );
    wxFileName  legacyProject( absPath + ".pro" );
    std::string boardPath = absPath + ".kicad_pcb";
    wxFileName  rulesFile( absPath + ".kicad_dru" );

    if( projectFile.Exists() )
        aSettingsManager.LoadProject( projectFile.GetFullPath() );
    else if( legacyProject.Exists() )
        aSettingsManager.LoadProject( legacyProject.GetFullPath() );

    aBoard = ReadBoardFromFileOrStream( boardPath );

    if( projectFile.Exists() || legacyProject.Exists() )
        aBoard->SetProject( &aSettingsManager.Prj() );

    auto m_DRCEngine = std::make_shared<DRC_ENGINE>( aBoard.get(), &aBoard->GetDesignSettings() );

    if( rulesFile.Exists() )
        m_DRCEngine->InitEngine( rulesFile );
    else
        m_DRCEngine->InitEngine( wxFileName() );

    aBoard->GetDesignSettings().m_DRCEngine = m_DRCEngine;
    aBoard->BuildListOfNets();
    aBoard->BuildConnectivity();
}


void FillZones( BOARD* m_board )
{
    TOOL_MANAGER toolMgr;
    toolMgr.SetEnvironment( m_board, nullptr, nullptr, nullptr, nullptr );

    BOARD_COMMIT       commit( &toolMgr );
    ZONE_FILLER        filler( m_board, &commit );
    std::vector<ZONE*> toFill;

    for( ZONE* zone : m_board->Zones() )
        toFill.push_back( zone );

    if( filler.Fill( toFill, false, nullptr ) )
        commit.Push( _( "Fill Zone(s)" ), false, false );
}


#define TEST( a, b )                                                                               \
    {                                                                                              \
        if( a != b )                                                                               \
            return a < b;                                                                          \
    }
#define TEST_PT( a, b )                                                                            \
    {                                                                                              \
        if( a.x != b.x )                                                                           \
            return a.x < b.x;                                                                      \
        if( a.y != b.y )                                                                           \
            return a.y < b.y;                                                                      \
    }


struct kitest_cmp_drawings
{
    FOOTPRINT::cmp_drawings fp_comp;

    bool operator()( const BOARD_ITEM* itemA, const BOARD_ITEM* itemB ) const
    {
        TEST( itemA->Type(), itemB->Type() );

        if( itemA->GetLayerSet() != itemB->GetLayerSet() )
            return itemA->GetLayerSet().Seq() < itemB->GetLayerSet().Seq();

        if( itemA->Type() == PCB_FP_TEXT_T )
        {
            const FP_TEXT* textA = static_cast<const FP_TEXT*>( itemA );
            const FP_TEXT* textB = static_cast<const FP_TEXT*>( itemB );

            TEST( textA->GetType(), textB->GetType() );
            TEST_PT( textA->GetPosition(), textB->GetPosition() );
            TEST( textA->GetTextAngle(), textB->GetTextAngle() );
        }

        return fp_comp( itemA, itemB );
    }
};


void CheckFootprint( const FOOTPRINT* expected, const FOOTPRINT* fp )
{
    CHECK_ENUM_CLASS_EQUAL( expected->Type(), fp->Type() );

    // TODO: validate those informations match the importer
    BOOST_CHECK_EQUAL( expected->GetPosition(), fp->GetPosition() );
    BOOST_CHECK_EQUAL( expected->GetOrientation(), fp->GetOrientation() );

    BOOST_CHECK_EQUAL( expected->GetReference(), fp->GetReference() );
    BOOST_CHECK_EQUAL( expected->GetValue(), fp->GetValue() );
    BOOST_CHECK_EQUAL( expected->GetDescription(), fp->GetDescription() );
    BOOST_CHECK_EQUAL( expected->GetKeywords(), fp->GetKeywords() );
    BOOST_CHECK_EQUAL( expected->GetAttributes(), fp->GetAttributes() );
    BOOST_CHECK_EQUAL( expected->GetFlag(), fp->GetFlag() );
    //BOOST_CHECK_EQUAL( expected->GetProperties(), fp->GetProperties() );
    BOOST_CHECK_EQUAL( expected->GetTypeName(), fp->GetTypeName() );

    // simple test if count matches
    BOOST_CHECK_EQUAL( expected->Pads().size(), fp->Pads().size() );
    BOOST_CHECK_EQUAL( expected->GraphicalItems().size(), fp->GraphicalItems().size() );
    BOOST_CHECK_EQUAL( expected->Zones().size(), fp->Zones().size() );
    BOOST_CHECK_EQUAL( expected->Groups().size(), fp->Groups().size() );
    BOOST_CHECK_EQUAL( expected->Models().size(), fp->Models().size() );

    std::set<PAD*, FOOTPRINT::cmp_pads> expectedPads( expected->Pads().begin(),
                                                      expected->Pads().end() );
    std::set<PAD*, FOOTPRINT::cmp_pads> fpPads( fp->Pads().begin(), fp->Pads().end() );
    for( auto itExpected = expectedPads.begin(), itFp = fpPads.begin();
         itExpected != expectedPads.end() && itFp != fpPads.end(); itExpected++, itFp++ )
    {
        CheckFpPad( *itExpected, *itFp );
    }

    std::set<BOARD_ITEM*, kitest_cmp_drawings> expectedGraphicalItems(
            expected->GraphicalItems().begin(), expected->GraphicalItems().end() );
    std::set<BOARD_ITEM*, kitest_cmp_drawings> fpGraphicalItems( fp->GraphicalItems().begin(),
                                                                 fp->GraphicalItems().end() );
    for( auto itExpected = expectedGraphicalItems.begin(), itFp = fpGraphicalItems.begin();
         itExpected != expectedGraphicalItems.end() && itFp != fpGraphicalItems.end();
         itExpected++, itFp++ )
    {
        BOOST_CHECK_EQUAL( ( *itExpected )->Type(), ( *itFp )->Type() );
        switch( ( *itExpected )->Type() )
        {
        case PCB_FP_TEXT_T:
        {
            const FP_TEXT* expectedText = static_cast<const FP_TEXT*>( *itExpected );
            const FP_TEXT* text = static_cast<const FP_TEXT*>( *itFp );

            CheckFpText( expectedText, text );
        }
        break;
        case PCB_FP_SHAPE_T:
        {
            const FP_SHAPE* expectedShape = static_cast<const FP_SHAPE*>( *itExpected );
            const FP_SHAPE* shape = static_cast<const FP_SHAPE*>( *itFp );

            CheckFpShape( expectedShape, shape );
        }
        break;
        /*case PCB_FP_DIM_ALIGNED_T: break;
            case PCB_FP_DIM_LEADER_T: break;
            case PCB_FP_DIM_CENTER_T: break;
            case PCB_FP_DIM_RADIAL_T: break;
            case PCB_FP_DIM_ORTHOGONAL_T: break;*/
        default: BOOST_ERROR( "KICAD_T not known" ); break;
        }
    }

    std::set<FP_ZONE*, FOOTPRINT::cmp_zones> expectedZones( expected->Zones().begin(),
                                                            expected->Zones().end() );
    std::set<FP_ZONE*, FOOTPRINT::cmp_zones> fpZones( fp->Zones().begin(), fp->Zones().end() );
    for( auto itExpected = expectedZones.begin(), itFp = fpZones.begin();
         itExpected != expectedZones.end() && itFp != fpZones.end(); itExpected++, itFp++ )
    {
        CheckFpZone( *itExpected, *itFp );
    }

    // TODO: Groups
}


void CheckFpPad( const PAD* expected, const PAD* pad )
{
    CHECK_ENUM_CLASS_EQUAL( expected->Type(), pad->Type() );

    BOOST_CHECK_EQUAL( expected->GetNumber(), pad->GetNumber() );
    CHECK_ENUM_CLASS_EQUAL( expected->GetAttribute(), pad->GetAttribute() );
    CHECK_ENUM_CLASS_EQUAL( expected->GetProperty(), pad->GetProperty() );
    CHECK_ENUM_CLASS_EQUAL( expected->GetShape(), pad->GetShape() );

    BOOST_CHECK_EQUAL( expected->IsLocked(), pad->IsLocked() );

    BOOST_CHECK_EQUAL( expected->GetPosition(), pad->GetPosition() );
    BOOST_CHECK_EQUAL( expected->GetSize(), pad->GetSize() );
    BOOST_CHECK_EQUAL( expected->GetOrientation(), pad->GetOrientation() );
    BOOST_CHECK_EQUAL( expected->GetDelta(), pad->GetDelta() );
    BOOST_CHECK_EQUAL( expected->GetOffset(), pad->GetOffset() );
    BOOST_CHECK_EQUAL( expected->GetDrillSize(), pad->GetDrillSize() );
    CHECK_ENUM_CLASS_EQUAL( expected->GetDrillShape(), pad->GetDrillShape() );

    BOOST_CHECK_EQUAL( expected->GetLayerSet(), pad->GetLayerSet() );

    BOOST_CHECK_EQUAL( expected->GetNetCode(), pad->GetNetCode() );
    BOOST_CHECK_EQUAL( expected->GetPinFunction(), pad->GetPinFunction() );
    BOOST_CHECK_EQUAL( expected->GetPinType(), pad->GetPinType() );
    BOOST_CHECK_EQUAL( expected->GetPadToDieLength(), pad->GetPadToDieLength() );
    BOOST_CHECK_EQUAL( expected->GetLocalSolderMaskMargin(), pad->GetLocalSolderMaskMargin() );
    BOOST_CHECK_EQUAL( expected->GetLocalSolderPasteMargin(), pad->GetLocalSolderPasteMargin() );
    BOOST_CHECK_EQUAL( expected->GetLocalSolderPasteMarginRatio(),
                       pad->GetLocalSolderPasteMarginRatio() );
    BOOST_CHECK_EQUAL( expected->GetLocalClearance(), pad->GetLocalClearance() );
    CHECK_ENUM_CLASS_EQUAL( expected->GetZoneConnection(), pad->GetZoneConnection() );
    BOOST_CHECK_EQUAL( expected->GetThermalSpokeWidth(), pad->GetThermalSpokeWidth() );
    BOOST_CHECK_EQUAL( expected->GetThermalSpokeAngle(), pad->GetThermalSpokeAngle() );
    BOOST_CHECK_EQUAL( expected->GetThermalGap(), pad->GetThermalGap() );
    BOOST_CHECK_EQUAL( expected->GetRoundRectRadiusRatio(), pad->GetRoundRectRadiusRatio() );
    BOOST_CHECK_EQUAL( expected->GetChamferRectRatio(), pad->GetChamferRectRatio() );
    BOOST_CHECK_EQUAL( expected->GetChamferPositions(), pad->GetChamferPositions() );
    BOOST_CHECK_EQUAL( expected->GetRemoveUnconnected(), pad->GetRemoveUnconnected() );
    BOOST_CHECK_EQUAL( expected->GetKeepTopBottom(), pad->GetKeepTopBottom() );

    // TODO: check complex pad shapes
    CHECK_ENUM_CLASS_EQUAL( expected->GetAnchorPadShape(), pad->GetAnchorPadShape() );
    CHECK_ENUM_CLASS_EQUAL( expected->GetCustomShapeInZoneOpt(), pad->GetCustomShapeInZoneOpt() );
}


void CheckFpText( const FP_TEXT* expected, const FP_TEXT* text )
{
    CHECK_ENUM_CLASS_EQUAL( expected->Type(), text->Type() );

    CHECK_ENUM_CLASS_EQUAL( expected->GetType(), text->GetType() );

    BOOST_CHECK_EQUAL( expected->IsLocked(), text->IsLocked() );

    BOOST_CHECK_EQUAL( expected->GetText(), text->GetText() );
    BOOST_CHECK_EQUAL( expected->GetPosition(), text->GetPosition() );
    BOOST_CHECK_EQUAL( expected->GetTextAngle(), text->GetTextAngle() );
    BOOST_CHECK_EQUAL( expected->IsKeepUpright(), text->IsKeepUpright() );

    BOOST_CHECK_EQUAL( expected->GetLayerSet(), text->GetLayerSet() );
    BOOST_CHECK_EQUAL( expected->IsVisible(), text->IsVisible() );

    BOOST_CHECK_EQUAL( expected->GetTextSize(), text->GetTextSize() );
    BOOST_CHECK_EQUAL( expected->GetLineSpacing(), text->GetLineSpacing() );
    BOOST_CHECK_EQUAL( expected->GetTextThickness(), text->GetTextThickness() );
    BOOST_CHECK_EQUAL( expected->IsBold(), text->IsBold() );
    BOOST_CHECK_EQUAL( expected->IsItalic(), text->IsItalic() );
    BOOST_CHECK_EQUAL( expected->GetHorizJustify(), text->GetHorizJustify() );
    BOOST_CHECK_EQUAL( expected->GetVertJustify(), text->GetVertJustify() );
    BOOST_CHECK_EQUAL( expected->IsMirrored(), text->IsMirrored() );
    BOOST_CHECK_EQUAL( expected->GetFontName(), text->GetFontName() ); // TODO: bold/italic setting?

    // TODO: render cache?
}


void CheckFpShape( const FP_SHAPE* expected, const FP_SHAPE* shape )
{
    CHECK_ENUM_CLASS_EQUAL( expected->Type(), shape->Type() );

    CHECK_ENUM_CLASS_EQUAL( expected->GetShape(), shape->GetShape() );

    BOOST_CHECK_EQUAL( expected->IsLocked(), shape->IsLocked() );

    BOOST_CHECK_EQUAL( expected->GetCenter(), shape->GetCenter() );
    BOOST_CHECK_EQUAL( expected->GetStart(), shape->GetStart() );
    BOOST_CHECK_EQUAL( expected->GetEnd(), shape->GetEnd() );
    BOOST_CHECK_EQUAL( expected->GetPosition(), shape->GetPosition() );
    BOOST_CHECK_EQUAL( expected->GetBezierC1(), shape->GetBezierC1() );
    BOOST_CHECK_EQUAL( expected->GetBezierC2(), shape->GetBezierC2() );

    CheckShapePolySet( &expected->GetPolyShape(), &shape->GetPolyShape() );

    BOOST_CHECK_EQUAL( expected->GetLayerSet(), shape->GetLayerSet() );

    BOOST_CHECK_EQUAL( expected->GetStroke().GetWidth(), shape->GetStroke().GetWidth() );
    CHECK_ENUM_CLASS_EQUAL( expected->GetStroke().GetPlotStyle(),
                            shape->GetStroke().GetPlotStyle() );
    CHECK_ENUM_CLASS_EQUAL( expected->GetFillMode(), shape->GetFillMode() );
}


void CheckFpZone( const FP_ZONE* expected, const FP_ZONE* zone )
{
    CHECK_ENUM_CLASS_EQUAL( expected->Type(), zone->Type() );

    BOOST_CHECK_EQUAL( expected->IsLocked(), zone->IsLocked() );

    BOOST_CHECK_EQUAL( expected->GetNetCode(), zone->GetNetCode() );
    BOOST_CHECK_EQUAL( expected->GetPriority(), zone->GetPriority() );
    CHECK_ENUM_CLASS_EQUAL( expected->GetPadConnection(), zone->GetPadConnection() );
    BOOST_CHECK_EQUAL( expected->GetLocalClearance(), zone->GetLocalClearance() );
    BOOST_CHECK_EQUAL( expected->GetMinThickness(), zone->GetMinThickness() );

    BOOST_CHECK_EQUAL( expected->GetLayerSet(), zone->GetLayerSet() );

    BOOST_CHECK_EQUAL( expected->IsFilled(), zone->IsFilled() );
    CHECK_ENUM_CLASS_EQUAL( expected->GetFillMode(), zone->GetFillMode() );
    BOOST_CHECK_EQUAL( expected->GetHatchThickness(), zone->GetHatchThickness() );
    BOOST_CHECK_EQUAL( expected->GetHatchGap(), zone->GetHatchGap() );
    BOOST_CHECK_EQUAL( expected->GetHatchOrientation(), zone->GetHatchOrientation() );
    BOOST_CHECK_EQUAL( expected->GetHatchSmoothingLevel(), zone->GetHatchSmoothingLevel() );
    BOOST_CHECK_EQUAL( expected->GetHatchSmoothingValue(), zone->GetHatchSmoothingValue() );
    BOOST_CHECK_EQUAL( expected->GetHatchBorderAlgorithm(), zone->GetHatchBorderAlgorithm() );
    BOOST_CHECK_EQUAL( expected->GetHatchHoleMinArea(), zone->GetHatchHoleMinArea() );
    BOOST_CHECK_EQUAL( expected->GetThermalReliefGap(), zone->GetThermalReliefGap() );
    BOOST_CHECK_EQUAL( expected->GetThermalReliefSpokeWidth(), zone->GetThermalReliefSpokeWidth() );
    BOOST_CHECK_EQUAL( expected->GetCornerSmoothingType(), zone->GetCornerSmoothingType() );
    BOOST_CHECK_EQUAL( expected->GetCornerRadius(), zone->GetCornerRadius() );
    CHECK_ENUM_CLASS_EQUAL( expected->GetIslandRemovalMode(), zone->GetIslandRemovalMode() );
    BOOST_CHECK_EQUAL( expected->GetMinIslandArea(), zone->GetMinIslandArea() );

    BOOST_CHECK_EQUAL( expected->GetIsRuleArea(), zone->GetIsRuleArea() );
    BOOST_CHECK_EQUAL( expected->GetDoNotAllowCopperPour(), zone->GetDoNotAllowCopperPour() );
    BOOST_CHECK_EQUAL( expected->GetDoNotAllowVias(), zone->GetDoNotAllowVias() );
    BOOST_CHECK_EQUAL( expected->GetDoNotAllowTracks(), zone->GetDoNotAllowTracks() );
    BOOST_CHECK_EQUAL( expected->GetDoNotAllowPads(), zone->GetDoNotAllowPads() );
    BOOST_CHECK_EQUAL( expected->GetDoNotAllowFootprints(), zone->GetDoNotAllowFootprints() );

    BOOST_CHECK_EQUAL( expected->GetZoneName(), zone->GetZoneName() );
    CHECK_ENUM_CLASS_EQUAL( expected->GetTeardropAreaType(), zone->GetTeardropAreaType() );
    BOOST_CHECK_EQUAL( expected->GetZoneName(), zone->GetZoneName() );

    CheckShapePolySet( expected->Outline(), zone->Outline() );
    // TODO: filled zones
}


void CheckShapePolySet( const SHAPE_POLY_SET* expected, const SHAPE_POLY_SET* polyset )
{
    BOOST_CHECK_EQUAL( expected->OutlineCount(), polyset->OutlineCount() );
    BOOST_CHECK_EQUAL( expected->TotalVertices(), polyset->TotalVertices() );

    // TODO: check all outlines and holes
}

} // namespace KI_TEST
