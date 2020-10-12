/*@ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 **
 **    Copyright (c) 2011-2015, JGU Mainz, Anton Popov, Boris Kaus
 **    All rights reserved.
 **
 **    This software was developed at:
 **
 **         Institute of Geosciences
 **         Johannes-Gutenberg University, Mainz
 **         Johann-Joachim-Becherweg 21
 **         55128 Mainz, Germany
 **
 **    project:    LaMEM
 **    filename:   bc.h
 **
 **    LaMEM is free software: you can redistribute it and/or modify
 **    it under the terms of the GNU General Public License as published
 **    by the Free Software Foundation, version 3 of the License.
 **
 **    LaMEM is distributed in the hope that it will be useful,
 **    but WITHOUT ANY WARRANTY; without even the implied warranty of
 **    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 **    See the GNU General Public License for more details.
 **
 **    You should have received a copy of the GNU General Public License
 **    along with LaMEM. If not, see <http://www.gnu.org/licenses/>.
 **
 **
 **    Contact:
 **        Boris Kaus       [kaus@uni-mainz.de]
 **        Anton Popov      [popov@uni-mainz.de]
 **
 **
 **    Main development team:
 **         Anton Popov      [popov@uni-mainz.de]
 **         Boris Kaus       [kaus@uni-mainz.de]
 **         Tobias Baumann
 **         Adina Pusok
 **         Arthur Bauville
 **
 ** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ @*/

//---------------------------------------------------------------------------
//........................... BOUNDARY CONDITIONS ...........................
//---------------------------------------------------------------------------
#ifndef __bc_h__
#define __bc_h__
//---------------------------------------------------------------------------

struct FB;
struct Scaling;
struct TSSol;
struct FDSTAG;
struct Marker;
struct DBMat;
struct JacRes;

//---------------------------------------------------------------------------
// index shift type
enum ShiftType
{
	_LOCAL_TO_GLOBAL_,
	_GLOBAL_TO_LOCAL_

};

//---------------------------------------------------------------------------
// Bezier block (rotating polygon moving along Bezier curve)
//---------------------------------------------------------------------------

struct BCBlock
{
	// path description
	PetscInt    npath;                        // number of path points of Bezier curve
	PetscScalar theta[  _max_path_points_  ]; // orientation angles at path points
	PetscScalar time [  _max_path_points_  ]; // times at path points
	PetscScalar path [6*_max_path_points_-4]; // Bezier curve path & control points (3*n-2)

	// block description
	PetscInt    npoly;                      // number of polygon vertices
	PetscScalar poly [2*_max_poly_points_]; // polygon coordinates
	PetscScalar bot, top;                   // bottom & top coordinates of the block

	// WARNING bottom coordinate should be advected (how? average?)
	// Top of the box can be assumed to be the free surface
	// sticky air nodes should never be constrained (this is easy to check)

};

//---------------------------------------------------------------------------

// setup data structures
PetscErrorCode BCBlockCreate(BCBlock *bcb, Scaling *scal, FB *fb);

// compute position along the path and rotation angle as a function of time
PetscErrorCode BCBlockGetPosition(BCBlock *bcb, PetscScalar t, PetscInt *f, PetscScalar x[]);

// compute current polygon coordinates
PetscErrorCode BCBlockGetPolygon(BCBlock *bcb, PetscScalar Xb[], PetscScalar *cpoly);

//---------------------------------------------------------------------------
// Dropping boxes (rectangular boxes moving with constant vertical velocity)
//---------------------------------------------------------------------------

struct DBox
{
	PetscInt    num;                   // number of boxes
	PetscInt 	advect_box;			   // advect box (=1) or not?
	PetscScalar bounds[6*_max_boxes_]; // box bounds
	PetscScalar zvel;                  // vertical velocity

} ;

//---------------------------------------------------------------------------

PetscErrorCode DBoxReadCreate(DBox *dbox, Scaling *scal, FB *fb);

//---------------------------------------------------------------------------

// boundary condition context
struct BCCtx
{
	//=====================================================================
	//
	// Boundary condition vectors contain prescribed DOF values:
	//
	//    *Internal points (marked with positive number in the index arrays)
	//        DBL_MAX   - active DOF flag
	//        otherwise - single-point constraint (SPC) value
	//
	//    *Boundary ghost point (marked with -1 in the index arrays)
	//        DBL_MAX   - free-slip (zero-flux) condition flag
	//        otherwise - two-point constraint (TPC) value
	//
	// Boundary ghost points require consistent setting of constraints
	// on the processor boundaries (since PETSc doesn't exchange boundary
	// ghost point values). Internal ghost points should be synchronized
	// after initializing the single-point constraints. Synchronization
	// can be skipped if all ghost points are initialized redundantly
	// on all the processes (DO THIS!).
	//
	// Single point constraints are additionally stored as lists
	// for constraining matrices and vectors. Matrices require global
	// index space, vectors require local index space.
	//
	// Global v-p index space can be either monolithic or block
	// Local v-p index space is ALWAYS coupled (since all solvers are coupled)
	//
	// NOTE! It may be worth storing TPC also as lists (for speedup).
	//=====================================================================


	FDSTAG   *fs;   // staggered grid
	TSSol    *ts;   // time stepping parameters
	Scaling  *scal; // scaling parameters
	DBMat    *dbm;  // material database
	JacRes   *jr;   // Jacobian-residual context (CROSS-REFERENCE!)

	// boundary conditions vectors (velocity, pressure, temperature)
	Vec bcvx, bcvy, bcvz, bcp, bcT; // local (ghosted)

	// single-point constraints
	ShiftType    stype;   // current index shift type
	PetscInt     numSPC;  // total number of constraints
	PetscInt    *SPCList; // local indices of SPC
	PetscScalar *SPCVals; // values of SPC

	// velocity
	PetscInt     vNumSPC;
	PetscInt    *vSPCList;
	PetscScalar *vSPCVals;

	// pressure
	PetscInt     pNumSPC;
	PetscInt    *pSPCList;
	PetscScalar *pSPCVals;

	// temperature
	PetscInt     tNumSPC;
	PetscInt    *tSPCList;
	PetscScalar *tSPCVals;

	// two-point constraints
//	PetscInt     numTPC;       // number of two-point constraints (TPC)
//	PetscInt    *TPCList;      // local indices of TPC (ghosted layout)
//	PetscInt    *TPCPrimeDOF;  // local indices of primary DOF (ghosted layout)
//	PetscScalar *TPCVals;      // values of TPC
//	PetscScalar *TPCLinComPar; // linear combination parameters

	//=====================
	// VELOCITY CONSTRAINTS
	//=====================

	// horizontal background strain-rate parameters
	PetscInt     ExxNumPeriods;
	PetscScalar  ExxTimeDelims [_max_periods_-1];
	PetscScalar  ExxStrainRates[_max_periods_  ];

	PetscInt     EyyNumPeriods;
	PetscScalar  EyyTimeDelims [_max_periods_-1];
	PetscScalar  EyyStrainRates[_max_periods_  ];

	// background strain rate reference point
	PetscScalar  BGRefPoint[3];

	// Bezier block
	PetscInt 	 nblocks;             // number of Bezier blocks
	BCBlock      blocks[_max_boxes_]; // BC block

	// dropping boxes
	DBox         dbox;

	// velocity inflow & outflow boundary condition
	PetscInt     face, phase,face_out;   // face (1-left 2-right 3-front 4-back) & phase identifiers
	PetscScalar  bot, top,relax_dist;      // bottom & top coordinates of the plate
	PetscScalar  velin, velout; // inflow & outflow velocities

	// velocity plume-like boundary condition
	PetscInt plume_like, plume_type;

	PetscInt plume_phase;
	PetscScalar plume_temperature;

	PetscInt plume_direction;
	PetscScalar coord_max, coord_min;

	PetscScalar center_plume[2];
	PetscScalar radius;

	PetscScalar velin_plume,delta_Time;

	// open boundary flag
	PetscInt     top_open;

	// no-slip boundary condition mask
	PetscInt     noslip[6];

	// fixed phase (no-flow condition)
	PetscInt     fixPhase;

	// fixed cells (no-flow condition)
	PetscInt       fixCell;
	unsigned char *fixCellFlag;

	//========================
	// TEMPERATURE CONSTRAINTS
	//========================

	// temperature on top and bottom boundaries and initial guess activation flag
	PetscScalar  Tbot, Ttop;
	PetscInt     initTemp;

	// optional gaussian temperature perturbation @ bottom
	PetscInt 	Tbot_gauss;
	PetscScalar Tbot_gauss_x0, Tbot_gauss_y0, Tbot_gauss_width, Tbot_gauss_maxT;

	//=====================
	// PRESSURE CONSTRAINTS
	//=====================

	// pressure on top and bottom boundaries and initial guess activation flag
	PetscScalar  pbot, ptop;
	PetscInt     initPres;

};
//---------------------------------------------------------------------------

// create boundary condition context
PetscErrorCode BCCreate(BCCtx *bc, FB *fb);

// read boundary condition context from restart database
PetscErrorCode BCReadRestart(BCCtx *bc, FILE *fp);

// write boundary condition context to restart database
PetscErrorCode BCWriteRestart(BCCtx *bc, FILE *fp);

// allocate internal vectors and arrays
PetscErrorCode BCCreateData(BCCtx *bc);

// destroy boundary condition context
PetscErrorCode BCDestroy(BCCtx *bc);

// read fixed cells from files in parallel
PetscErrorCode BCReadFixCell(BCCtx *bc, FB *fb);

// apply ALL boundary conditions
PetscErrorCode BCApply(BCCtx *bc);

// apply SPC to global solution vector
PetscErrorCode BCApplySPC(BCCtx *bc);

// shift indices of constrained nodes
PetscErrorCode BCShiftIndices(BCCtx *bc, ShiftType stype);

//---------------------------------------------------------------------------
// Specific constraints
//---------------------------------------------------------------------------

// apply pressure constraints
PetscErrorCode BCApplyPres(BCCtx *bc);

// apply temperature constraints
PetscErrorCode BCApplyTemp(BCCtx *bc);

// apply default velocity constraints on the boundaries
PetscErrorCode BCApplyVelDefault(BCCtx *bc);

// apply Bezier blocks
PetscErrorCode BCApplyBezier(BCCtx *bc);

// apply inflow/outflow boundary velocities
PetscErrorCode BCApplyBoundVel(BCCtx *bc);

// apply dropping boxes
PetscErrorCode BCApplyDBox(BCCtx *bc);

// constraint all cells containing phase
PetscErrorCode BCApplyPhase(BCCtx *bc);

// constrain cells marked in files in parallel
PetscErrorCode BCApplyCells(BCCtx *bc);

// create SPC constraint lists
PetscErrorCode BCListSPC(BCCtx *bc);

// apply two-point constraints on the boundaries
PetscErrorCode BCApplyVelTPC(BCCtx *bc);

// apply plume_open_boundary condition

PetscErrorCode BC_Plume_inflow(BCCtx *bc);

//---------------------------------------------------------------------------
// Service functions
//---------------------------------------------------------------------------

// get current background strain rates & reference point coordinates
PetscErrorCode BCGetBGStrainRates(
		BCCtx       *bc,
		PetscScalar *Exx_,
		PetscScalar *Eyy_,
		PetscScalar *Ezz_,
		PetscScalar *Rxx_,
		PetscScalar *Ryy_,
		PetscScalar *Rzz_);

// stretch staggered grid if background strain rates are defined
PetscErrorCode BCStretchGrid(BCCtx *bc);

// change phase of inflow markers if velocity boundary condition is defined
PetscErrorCode BCOverridePhase(BCCtx *bc, PetscInt cellID, Marker *P);

//---------------------------------------------------------------------------
// MACROS
//---------------------------------------------------------------------------

#define LIST_SPC(bc, list, vals, cnt, iter)\
	if(bc[k][j][i] != DBL_MAX) { list[cnt] = iter; vals[cnt] = bc[k][j][i]; cnt++; }

//---------------------------------------------------------------------------
#endif
