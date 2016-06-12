/* OTGrammar_enums.h
 *
 * Copyright (C) 2006-2008 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * pb 2008/05/31
 */

enum_begin (OTGrammar_DECISION_STRATEGY, OptimalityTheory)   /* This cannot be called "None" !!! */
	enum (HarmonicGrammar)
	enum (LinearOT)
	enum (ExponentialHG)
	enum (MaximumEntropy)
	enum (PositiveHG)
	enum (ExponentialMaximumEntropy)
enum_end (OTGrammar_DECISION_STRATEGY)

/* End of file Experiment_enums.h */
