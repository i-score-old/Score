/** @file
 *
 * @ingroup scoreLibrary
 *
 * @brief the Score Application Programming Interface
 *
 * @details The Score API allows to use Score inside another application @n@n
 *
 * @see TTScore
 *
 * @authors Théo de la Hogue & Clément Bossut
 *
 * @copyright Copyright © 2013, Théo de la Hogue & Clément Bossut @n
 * This code is licensed under the terms of the "CeCILL-C" @n
 * http://www.cecill.info
 */

#ifndef __TT_SCORE_H__
#define __TT_SCORE_H__

#include "TTScoreIncludes.h"
#include "Expression.h"
#include "TTTimeCondition.h"
#include "TTTimeContainer.h"
#include "TTTimeEvent.h"
#include "TTTimeProcess.h"

#include "TTScore.test.h"

#if 0
#pragma mark -
#pragma mark Initialisation
#endif

/** Initialise Score framework */
void TTSCORE_EXPORT TTScoreInit();

#endif  // __TT_SCORE_H__
