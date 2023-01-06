/***************************************************************************
    costcalculator.cpp
    ---------------------
    begin                : November 2009
    copyright            : (C) 2009 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "layer.h"
#include <json.hpp>
#include "pal.h"
#include "feature.h"
#include "geomfunction.h"
#include "labelposition.h"
#include "util.h"
#include "costcalculator.h"
#include <cmath>
#include <cfloat>

using namespace pal;

bool CostCalculator::candidateSortGrow( const std::unique_ptr< LabelPosition > &c1, const std::unique_ptr< LabelPosition > &c2 )
{
  // literally just a comparison function used for sorting
  // printf("CandidateSortGrow \n");

  // nlohmann::json debug = {
  //   {"c1", c1->cost()},
  //   {"c2", c2->cost()},
  // };
  // std::cout << debug << std::endl;
  return c1->cost() < c2->cost();
}

void CostCalculator::addObstacleCostPenalty( LabelPosition *lp, FeaturePart *obstacle, Pal *pal )
{
  printf("==================addObstacleCostPenalty \n");
  int n = 0;
  double dist;
  const double distlabel = lp->feature->getLabelDistance();
  
  printf("CostCalculator::addObstacleCostPenalty lp->getId() == %d, obstacle->featureId=%lld",
         lp->getId(), obstacle->featureId());

  switch ( obstacle->getGeosType() )
  {
    case GEOS_POINT:
      printf("case GEOS_POINT \n");
      dist = lp->getDistanceToPoint( obstacle->x[0], obstacle->y[0] );
      if ( dist < 0 )
        n = 2;
      else if ( dist < distlabel )
        //note this never happens at the moment - points are not obstacles if they don't fall
        //within the label
        n = 1;
      else
        n = 0;
      break;

    case GEOS_LINESTRING:
      printf("case GEOS_LINESTRING \n");
      // Is one of label's borders crossing the line ?
      n = ( lp->crossesLine( obstacle ) ? 1 : 0 );
      break;

    case GEOS_POLYGON:
      printf("case GEOS_POLYGON \n");
      // behavior depends on obstacle avoid type
      switch ( obstacle->layer()->obstacleType() )
      {
        case QgsLabelObstacleSettings::PolygonInterior:
          // n ranges from 0 -> 12
          n = lp->polygonIntersectionCost( obstacle );
          break;
        case QgsLabelObstacleSettings::PolygonBoundary:
          // penalty may need tweaking, given that interior mode ranges up to 12
          n = ( lp->crossesBoundary( obstacle ) ? 6 : 0 );
          break;
        case QgsLabelObstacleSettings::PolygonWhole:
          // n is either 0 or 12
          n = ( lp->intersectsWithPolygon( obstacle ) ? 12 : 0 );
          break;
      }

      break;
  }

  //scale cost by obstacle's factor
  const double obstacleCost = obstacle->obstacleSettings().factor() * double( n );
  if ( n > 0 )
    lp->setConflictsWithObstacle( true );

  switch ( pal->placementVersion() )
  {
    case QgsLabelingEngineSettings::PlacementEngineVersion1:
      break;

    case QgsLabelingEngineSettings::PlacementEngineVersion2:
    {
      // obstacle factor is from 0 -> 2, label priority is from 1 -> 0. argh!
      const double priority = 2 * ( 1 - lp->feature->calculatePriority() );
      const double obstaclePriority = obstacle->obstacleSettings().factor();

      // if feature priority is < obstaclePriorty, there's a hard conflict...
      printf("  check priority n=%d, priority=%lf, obstaclePriority=%lf\n", n, priority, obstaclePriority);
      if ( n > 0 && ( priority < obstaclePriority && !qgsDoubleNear( priority, obstaclePriority, 0.001 ) ) )
      {
        printf("  setHasHardObstacleConflict true\n");
        lp->setHasHardObstacleConflict( true );
      }
      break;
    }
  }

  // label cost is penalized
  printf("addObstacleCost %d, %lf\n", lp->getId(), obstacleCost);
  lp->setCost( lp->cost() + obstacleCost );
}
//internal function
void CostCalculator::calculateCandidatePolygonRingDistanceCosts( std::vector< std::unique_ptr< LabelPosition > > &lPos, double bbx[4], double bby[4] )
{
  // first we calculate the ring distance cost for all candidates for this feature. We then use the range
  // of distance costs to calculate a standardised scaling for the costs
  printf("====================calculateCandidatePolygonRingDistanceCosts \n");
  QHash< LabelPosition *, double > polygonRingDistances;
  double minCandidateRingDistance = std::numeric_limits< double >::max();
  double maxCandidateRingDistance = std::numeric_limits< double >::lowest();
  for ( const std::unique_ptr< LabelPosition > &pos : lPos )
  {
    const double candidatePolygonRingDistance = calculatePolygonRingDistance( pos.get(), bbx, bby );

    minCandidateRingDistance = std::min( minCandidateRingDistance, candidatePolygonRingDistance );
    maxCandidateRingDistance = std::max( maxCandidateRingDistance, candidatePolygonRingDistance );

    polygonRingDistances.insert( pos.get(), candidatePolygonRingDistance );
  }

  // define the cost's range, if range is too small, just ignore the ring distance cost
  const double costRange = maxCandidateRingDistance - minCandidateRingDistance;
  if ( costRange <= EPSILON )
    return;

  const double normalizer = 0.0020 / costRange;

  // adjust cost => the best is 0, the worst is 0.002
  // others are set proportionally between best and worst
  for ( std::unique_ptr< LabelPosition > &pos : lPos )
  {
    const double polygonRingDistanceCost = polygonRingDistances.value( pos.get() );
    pos->setCost( pos->cost() + 0.002 - ( polygonRingDistanceCost - minCandidateRingDistance ) * normalizer );
  }
}
//internal function
void CostCalculator::calculateCandidatePolygonCentroidDistanceCosts( pal::FeaturePart *feature, std::vector<std::unique_ptr<LabelPosition> > &lPos )
{
  printf("=======================calculateCandidatePolygonCentroidDistanceCosts \n");
  double cx, cy;
  feature->getCentroid( cx, cy );

  // first we calculate the centroid distance cost for all candidates for this feature. We then use the range
  // of distance costs to calculate a standardised scaling for the costs
  QHash< LabelPosition *, double > polygonCentroidDistances;
  double minCandidateCentroidDistance = std::numeric_limits< double >::max();
  double maxCandidateCentroidDistance = std::numeric_limits< double >::lowest();
  for ( std::unique_ptr< LabelPosition > &pos : lPos )
  {
    const double lPosX = ( pos->x[0] + pos->x[2] ) / 2.0;
    const double lPosY = ( pos->y[0] + pos->y[2] ) / 2.0;

    const double candidatePolygonCentroidDistance = std::sqrt( ( cx - lPosX ) * ( cx - lPosX ) + ( cy - lPosY ) * ( cy - lPosY ) );

    minCandidateCentroidDistance  = std::min( minCandidateCentroidDistance, candidatePolygonCentroidDistance );
    maxCandidateCentroidDistance = std::max( maxCandidateCentroidDistance, candidatePolygonCentroidDistance );

    polygonCentroidDistances.insert( pos.get(), candidatePolygonCentroidDistance );
  }

  // define the cost's range, if range is too small, just ignore the ring distance cost
  const double costRange = maxCandidateCentroidDistance - minCandidateCentroidDistance;
  if ( costRange <= EPSILON )
    return;

  const double normalizer = 0.001 / costRange;

  // adjust cost => the closest is 0, the furthest is 0.001
  // others are set proportionally between best and worst
  // NOTE: centroid cost range may need adjusting with respect to ring distance range!
  for ( std::unique_ptr< LabelPosition > &pos : lPos )
  {
    const double polygonCentroidDistance = polygonCentroidDistances.value( pos.get() );
    pos->setCost( pos->cost() + ( polygonCentroidDistance - minCandidateCentroidDistance ) * normalizer );
  }
}
//internal function
double CostCalculator::calculatePolygonRingDistance( LabelPosition *candidate, double bbx[4], double bby[4] )
{
  printf("=====================calculatePolygonRingDistance \n");
  // TODO 1: Consider whether distance calculation should use min distance to the candidate rectangle
  // instead of just the center
  CandidatePolygonRingDistanceCalculator ringDistanceCalculator( candidate );

  // first, check max distance to outside of polygon
  // TODO 2: there's a bug here -- if a candidate's center falls outside the polygon, then a larger
  // distance to the polygon boundary is being considered as the best placement. That's clearly wrong --
  // if any part of label candidate sits outside the polygon, we should first prefer candidates which sit
  // entirely WITHIN the polygon, or failing that, candidates which are CLOSER to the polygon boundary, not further from it!
  ringDistanceCalculator.addRing( candidate->feature );

  // prefer candidates further from the outside of map rather then those close to the outside of the map
  // TODO 3: should be using the actual map boundary here, not the bounding box
  const PointSet extent( 4, bbx, bby );
  ringDistanceCalculator.addRing( &extent );

  // prefer candidates which further from interior rings (holes) of the polygon
  for ( int i = 0; i < candidate->feature->getNumSelfObstacles(); ++i )
  {
    ringDistanceCalculator.addRing( candidate->feature->getSelfObstacle( i ) );
  }

  return ringDistanceCalculator.minimumDistance();
}

void printCandidates(Feats *feat) {
  std::size_t count = 0;
  for( count = 0; count < feat->candidates.size(); count++) {
    nlohmann::json candidateJson = {
      {"id", feat->candidates[count]->getId()},
      {"cost", feat->candidates[count]->cost()},
      //{"geometry", feat->candidates[count]->feature},
    };
    std::cout << "candidate" << candidateJson << std::endl;
    //std::cout << "id: " << feat->candidates[count]->getId() <<" candidate cost " << feat->candidates[count]->cost() << std::endl;
  }
}

void CostCalculator::finalizeCandidatesCosts( Feats *feat, double bbx[4], double bby[4] )
{
  printf("=============================finalizeCandidatesCosts %lld\n", feat->feature->featureId());
  printf("feat priority = %f \n", feat->priority);
  std::cout << "feature geos type = " << feat->feature->getGeosType() << std::endl;
  printf("bbx = %f %f %f %f and bby= %f %f %f %f \n",bbx[0], bbx[1], bbx[2], bbx[3], bby[0], bby[1], bby[2], bby[3]);

  nlohmann::json debug = {
     {"candidate Sizes", feat->candidates.size()},
     {"feature type", feat->feature->getGeosType()},
     {"feature length", feat->feature->length()},
     {"feature area", feat->feature->area()},
     {"Arrangement", feat->feature->layer()->arrangement()},
   };
  std::cout << "Feature details " << debug << std::endl;

  // sort candidates list, best label to worst
  std::sort( feat->candidates.begin(), feat->candidates.end(), candidateSortGrow );
  printCandidates(feat);
  
  // Original nonsense comment from pal library:
  // "try to exclude all conflitual labels (good ones have cost < 1 by pruning)"
  // my interpretation: it appears this scans through the candidates and chooses some threshold
  // based on the candidate cost, after which all remaining candidates are simply discarded
  // my suspicion: I don't think this is needed (or wanted) here, and is reflective of the fact
  // that obstacle costs are too low vs inferior candidate placement costs (i.e. without this,
  // an "around point" label would rather be placed over an obstacle then be placed in a position > 6 o'clock
  double discrim = 0.0;
  std::size_t stop = 0;
  do
  {
    discrim += 1.0;
    for ( stop = 0; stop < feat->candidates.size() && feat->candidates[ stop ]->cost() < discrim; stop++ )
      ;
  }
  while ( stop == 0 && discrim < feat->candidates.back()->cost() + 2.0 );
  printf("discrim = %f \n", discrim);
  // THIS LOOKS SUSPICIOUS -- it clamps all costs to a fixed value??
  if ( discrim > 1.5 )
  {
    for ( std::size_t k = 0; k < stop; k++ )
      feat->candidates[ k ]->setCost( 0.0021 );
  }
  
  printf("stop = %lu and size = %lu\n", stop, feat->candidates.size());
  printf("After Discrim \n");
  printCandidates(feat);

  if ( feat->candidates.size() > stop )
  {
    feat->candidates.resize( stop ); 
  }

  printf("After resize \n");
  printCandidates(feat);

  // Sets costs for candidates of polygon

  if ( feat->feature->getGeosType() == GEOS_POLYGON )
  {
    printf("Special polygon functionality \n");
    const Qgis::LabelPlacement arrangement = feat->feature->layer()->arrangement();
    //printf("arrangement = %d \n", arrangement);
    if ( arrangement == Qgis::LabelPlacement::Free || arrangement == Qgis::LabelPlacement::Horizontal )
    {
      // prefer positions closer to the pole of inaccessibilities
      calculateCandidatePolygonRingDistanceCosts( feat->candidates, bbx, bby );
      // ...of these, prefer positions closer to the overall polygon centroid
      calculateCandidatePolygonCentroidDistanceCosts( feat->feature, feat->candidates );
    }
  }

  // add size penalty (small lines/polygons get higher cost)
  feat->feature->addSizePenalty( feat->candidates, bbx, bby );
  printf("after penalty \n");
  printCandidates(feat);
}

CandidatePolygonRingDistanceCalculator::CandidatePolygonRingDistanceCalculator( LabelPosition *candidate )
  : mPx( ( candidate->x[0] + candidate->x[2] ) / 2.0 )
  , mPy( ( candidate->y[0] + candidate->y[2] ) / 2.0 )
{
  printf("====================CandidatePolygonRingDistanceCalculator \n");
}

void CandidatePolygonRingDistanceCalculator::addRing( const pal::PointSet *ring )
{
  printf("==================addRing \n");
  const double d = ring->minDistanceToPoint( mPx, mPy );
  if ( d < mMinDistance )
  {
    mMinDistance = d;
  }
}

double CandidatePolygonRingDistanceCalculator::minimumDistance() const
{
  printf("=======================minimumDistance \n");
  return mMinDistance;
}
