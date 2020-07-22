/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDSpline
//

ImplementObjectType(FCDSpline);

FCDSpline::FCDSpline(FCDocument* document)
:	FCDObject(document)
{
	form = FUDaeSplineForm::OPEN;
}

FCDSpline::~FCDSpline()
{
	cvs.clear();
}

FCDSpline* FCDSpline::Clone(FCDSpline* clone) const
{
	if (clone == NULL) return NULL;

	clone->cvs = cvs;
	clone->name = name;
	clone->form = form;

	return clone;
}

bool FCDSpline::LoadFromXML(xmlNode* splineNode)
{
	// Read the curve closed attribute
	SetClosed(FUStringConversion::ToBoolean(ReadNodeProperty(splineNode, DAE_CLOSED_ATTRIBUTE)));

	// Read in the <control_vertices> element, which define the base type for this curve
	xmlNode* controlVerticesNode = FindChildByType(splineNode, DAE_CONTROL_VERTICES_ELEMENT);
	if (controlVerticesNode == NULL)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_CONTROL_VERTICES_MISSING, splineNode->line);
		return false;
	}

	// Read in the <control_vertices> inputs.
	xmlNodeList inputElements;
	FindChildrenByType(controlVerticesNode, DAE_INPUT_ELEMENT, inputElements);
	for (size_t i = 0; i < inputElements.size(); i++)
	{
		xmlNode* inputNode = inputElements[i];
		fm::string sourceId = ReadNodeProperty(inputNode, DAE_SOURCE_ATTRIBUTE);
		if (sourceId.empty()) { FUError::Error(FUError::ERROR, FUError::ERROR_DEFAULT_ERROR); return false; }
		xmlNode* sourceNode = FindChildById(splineNode, sourceId);
		if (sourceNode == NULL) { FUError::Error(FUError::ERROR, FUError::ERROR_DEFAULT_ERROR); return false; }

		if (IsEquivalent(ReadNodeProperty(inputNode, DAE_SEMANTIC_ATTRIBUTE), DAE_CVS_SPLINE_INPUT))
		{
			// Read in the spline control points.
			ReadSource(sourceNode, cvs);
		}
	}

	return true;
}

xmlNode* FCDSpline::WriteToXML(xmlNode* parentNode, const fm::string& parentId, const fm::string& splineId) const
{
	// Create the spline node with its 'closed' attribute
    xmlNode* splineNode = AddChild(parentNode, DAE_SPLINE_ELEMENT);
	AddAttribute(splineNode, DAE_CLOSED_ATTRIBUTE, IsClosed());

	// Write out the control point source
	FUSStringBuilder controlPointSourceId(parentId); controlPointSourceId += "-cvs-" + splineId;
	AddSourcePosition(splineNode, controlPointSourceId.ToCharPtr(), cvs);

	// Write out the <control_vertices> element and its inputs
	xmlNode* verticesNode = AddChild(splineNode, DAE_CONTROL_VERTICES_ELEMENT);
	AddInput(verticesNode, controlPointSourceId.ToCharPtr(), DAE_CVS_SPLINE_INPUT);

	// Write out the spline type
	xmlNode* extraNode = AddExtraTechniqueChild(splineNode, DAE_FCOLLADA_PROFILE);
	AddChild(extraNode, DAE_TYPE_ATTRIBUTE, FUDaeSplineType::ToString(GetSplineType()));
	return splineNode;
}

//
// FCDLinearSpline
//

ImplementObjectType(FCDLinearSpline);

FCDLinearSpline::FCDLinearSpline(FCDocument* document)
:	FCDSpline(document)
{
}

FCDLinearSpline::~FCDLinearSpline()
{
}

void FCDLinearSpline::ToBezier(FCDBezierSpline& bezier)
{
	if (!IsValid()) return;
        
    // clear the given spline
    bezier.ClearCVs();

	size_t count = cvs.size();
	bool closed = IsClosed();

	if (closed) 
	{
		bezier.SetClosed(true);
	}

	for (size_t i = 0; i < count; i++)
	{
		FMVector3& cv = cvs[i];
		if (!closed && (i == 0 || i == count - 1))
		{
			// first and last CV on an open spline, 2 times
			bezier.AddCV(cv);
			bezier.AddCV(cv);
		}
		else
		{
			// in between CVs, three times
			bezier.AddCV(cv);
			bezier.AddCV(cv);
			bezier.AddCV(cv);
		}
	}
}

bool FCDLinearSpline::IsValid() const
{
	return cvs.size() >= 2;
}

bool FCDLinearSpline::LoadFromXML(xmlNode* splineNode)
{
	bool status = Parent::LoadFromXML(splineNode);
	if (!status) return false;
	status = IsValid();
	return status;
}

//
// FCDBezierSpline
//

ImplementObjectType(FCDBezierSpline);

FCDBezierSpline::FCDBezierSpline(FCDocument* document)
:	FCDSpline(document)
{
}

FCDBezierSpline::~FCDBezierSpline()
{
}

void FCDBezierSpline::ToNURBs(FCDNURBSSplineList &toFill) const
{
	// calculate the number of nurb segments
	bool closed = IsClosed();
	int cvsCount = (int)cvs.size();
	int nurbCount = (!closed) ? ((cvsCount - 1) / 3) : (cvsCount / 3);

	// if the spline is closed, ignore the first CV as it is the in-tangent of the first knot
	size_t curCV = (!closed) ? 0 : 1;
	int lastNurb = (!closed) ? nurbCount : (nurbCount - 1);

	for (int i = 0; i < lastNurb; i++)
	{
		FCDNURBSSpline* nurb = new FCDNURBSSpline(const_cast<FCDocument*>(GetDocument()));
		nurb->SetDegree(3);

		// add (degree + 1) CVs to the nurb
		for (size_t i = 0; i < (3+1); i++)
		{
			nurb->AddCV(cvs[curCV++], 1.0f);
		}

		// the last CVs will be the starting point of the next nurb segment
		curCV--;

		// add (degree+1) knots on each side of the knot vector
		for (size_t i = 0; i < (3+1); i++) nurb->AddKnot(0.0f);
		for (size_t i = 0; i < (3+1); i++) nurb->AddKnot(1.0f);

		// nurbs created this way are never closed
		nurb->SetClosed(false);

		// add the nurb
		toFill.push_back(nurb);
	}

	if (closed)
	{
		// we still have one NURB to create
		FCDNURBSSpline* nurb = new FCDNURBSSpline(const_cast<FCDocument*>(GetDocument()));
		nurb->SetDegree(3);

		nurb->AddCV(cvs[cvsCount - 2], 1.0f); // the last knot position
		nurb->AddCV(cvs[cvsCount - 1], 1.0f); // the last knot out-tangent
		nurb->AddCV(cvs[0			 ], 1.0f); // the first knot in-tangent
		nurb->AddCV(cvs[1			 ], 1.0f); // the first knot position

		// add (degree+1) knots on each side of the knot vector
		for (size_t i = 0; i < (3+1); i++) nurb->AddKnot(0.0f);
		for (size_t i = 0; i < (3+1); i++) nurb->AddKnot(1.0f);

		// nurbs created this way are never closed
		nurb->SetClosed(false);

		// add the nurb
		toFill.push_back(nurb);
	}
}

bool FCDBezierSpline::IsValid() const
{
	bool s = true;
	if (cvs.size() == 0)
	{
		s = !FUError::Error(FUError::WARNING, FUError::WARNING_SPLINE_CONTROL_INPUT_MISSING);
	}
	return s;
}

bool FCDBezierSpline::LoadFromXML(xmlNode* splineNode)
{
	bool status = Parent::LoadFromXML(splineNode);
	if (!status) return false;
	status = IsValid();
	return status;
}

//
// FCDNURBSSpline
//

ImplementObjectType(FCDNURBSSpline);

FCDNURBSSpline::FCDNURBSSpline(FCDocument* document)
:	FCDSpline(document)
{
}

FCDNURBSSpline::~FCDNURBSSpline()
{
	weights.clear();
	knots.clear();
}

FCDSpline* FCDNURBSSpline::Clone(FCDSpline* _clone) const
{
	FCDNURBSSpline* clone = NULL;
	if (_clone == NULL) return NULL;
	else if (_clone->HasType(FCDNURBSSpline::GetClassType())) clone = (FCDNURBSSpline*) _clone;

	Parent::Clone(_clone);

	if (clone != NULL)
	{
		// Clone the NURBS-specific spline data
		clone->degree = degree;
		clone->weights = weights;
		clone->knots = knots;
	}

	return _clone;
}

bool FCDNURBSSpline::AddCV(const FMVector3& cv, float weight)
{ 
	if (weight < 0.0f) return false;

	cvs.push_back(cv);
	weights.push_back(weight);
	return true;
}

bool FCDNURBSSpline::IsValid() const
{
	bool s = true;
	if (cvs.size() == 0)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_SPLINE_CONTROL_INPUT_MISSING);
		s = false;
	}

	if (cvs.size() != weights.size())
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_INVALID_CVS_WEIGHTS);
		s = false;
	}

	if (cvs.size() != knots.size() - degree - 1)
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_INVALID_SPLINE);
		s = false;
	}

	return s;
}

bool FCDNURBSSpline::LoadFromXML(xmlNode* splineNode)
{
	bool status = Parent::LoadFromXML(splineNode);
	if (!status) return false;

	xmlNode* extraNode = FindChildByType(splineNode, DAE_EXTRA_ELEMENT);
	if (extraNode == NULL) { FUError::Error(FUError::ERROR, FUError::ERROR_DEFAULT_ERROR); return status; }
	xmlNode* fcolladaNode = FindTechnique(extraNode, DAE_FCOLLADA_PROFILE);
	if (fcolladaNode == NULL) { FUError::Error(FUError::ERROR, FUError::ERROR_DEFAULT_ERROR); return status; }

	// Read in the NURBS degree
	xmlNode* degreeNode = FindChildByType(fcolladaNode, DAE_DEGREE_ATTRIBUTE);
	degree = (degreeNode != NULL) ? FUStringConversion::ToUInt32(ReadNodeContentDirect(degreeNode)) : 3;

	// Read in the <control_vertices> element, which define the base type for this curve
	xmlNode* controlVerticesNode = FindChildByType(splineNode, DAE_CONTROL_VERTICES_ELEMENT);
	if (controlVerticesNode == NULL)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_CONTROL_VERTICES_MISSING, splineNode->line);
		return status;
	}

	// read the sources
	xmlNodeList inputElements;
	FindChildrenByType(controlVerticesNode, DAE_INPUT_ELEMENT, inputElements);

	for (size_t i = 0; i < inputElements.size(); i++)
	{
		xmlNode* inputNode = inputElements[i];
		fm::string sourceId = ReadNodeProperty(inputNode, DAE_SOURCE_ATTRIBUTE).substr(1);
		if (sourceId.empty()) { FUError::Error(FUError::ERROR, FUError::ERROR_DEFAULT_ERROR); return status; }
		xmlNode* sourceNode = FindChildById(splineNode, sourceId);
		if (sourceNode == NULL) { FUError::Error(FUError::ERROR, FUError::ERROR_DEFAULT_ERROR); return status; }

		else if (IsEquivalent(ReadNodeProperty(inputNode, DAE_SEMANTIC_ATTRIBUTE), DAE_KNOT_SPLINE_INPUT))
		{
			ReadSource(sourceNode, knots);
		}
		else if (IsEquivalent(ReadNodeProperty(inputNode, DAE_SEMANTIC_ATTRIBUTE), DAE_WEIGHT_SPLINE_INPUT))
		{
			ReadSource(sourceNode, weights);
		}
	}

	status &= IsValid();

	return status;
}

xmlNode* FCDNURBSSpline::WriteToXML(xmlNode* parentNode, const fm::string& parentId, const fm::string& splineId) const
{
	// Create the <spline> XML tree node and set its 'closed' attribute.
	xmlNode* splineNode = AddChild(parentNode, DAE_SPLINE_ELEMENT);
	AddAttribute(splineNode, DAE_CLOSED_ATTRIBUTE, IsClosed());

	// Write out the control point, weight and knot sources
	FUSStringBuilder controlPointSourceId(parentId); controlPointSourceId += "-cvs-" + splineId;
	AddSourcePosition(splineNode, controlPointSourceId.ToCharPtr(), cvs);
	FUSStringBuilder weightSourceId(parentId); weightSourceId += "-weights-" + splineId;
	AddSourceFloat(splineNode, weightSourceId.ToCharPtr(), weights, "WEIGHT");
	FUSStringBuilder knotSourceId(parentId); knotSourceId += "-knots-" + splineId;
	AddSourceFloat(splineNode, knotSourceId.ToCharPtr(), knots, "KNOT");

	// Write out the <control_vertices> element and its inputs
	xmlNode* verticesNode = AddChild(splineNode, DAE_CONTROL_VERTICES_ELEMENT);
	AddInput(verticesNode, controlPointSourceId.ToCharPtr(), DAE_CVS_SPLINE_INPUT);
	AddInput(verticesNode, weightSourceId.ToCharPtr(), DAE_WEIGHT_SPLINE_INPUT);
	AddInput(verticesNode, knotSourceId.ToCharPtr(), DAE_KNOT_SPLINE_INPUT);

	// Write out the <extra> information: the spline type and degree.
	xmlNode* extraNode = AddExtraTechniqueChild(splineNode,DAE_FCOLLADA_PROFILE);
	AddChild(extraNode, DAE_TYPE_ATTRIBUTE, FUDaeSplineType::ToString(GetSplineType()));
	AddChild(extraNode, DAE_DEGREE_ATTRIBUTE, FUStringConversion::ToString(degree));
	return splineNode;
}

//
// FCDGeometrySpline
//

ImplementObjectType(FCDGeometrySpline);

FCDGeometrySpline::FCDGeometrySpline(FCDocument* document, FCDGeometry* _parent, FUDaeSplineType::Type _type)
:	FCDObject(document), parent(_parent), type(_type)
{
}

FCDGeometrySpline::~FCDGeometrySpline()
{
	parent = NULL;
}

FCDGeometrySpline* FCDGeometrySpline::Clone(FCDGeometrySpline* clone) const
{
	if (clone == NULL) clone = new FCDGeometrySpline(const_cast<FCDocument*>(GetDocument()), NULL, type);

	// Clone the spline set.
	for (FCDSplineContainer::const_iterator it = splines.begin(); it != splines.end(); ++it)
	{
		FCDSpline* cloneSpline = clone->AddSpline(type);
		(*it)->Clone(cloneSpline);
	}

	return clone;
}

bool FCDGeometrySpline::SetType(FUDaeSplineType::Type _type)
{
	for (size_t i = 0; i < splines.size(); ++i)
	{
		if (splines[i] == NULL || splines[i]->GetSplineType() != _type)
		{
			return false;
		}
	}

	type = _type;
	SetDirtyFlag();
	return true;
}

FCDSpline* FCDGeometrySpline::AddSpline(FUDaeSplineType::Type type)
{
	// Retrieve the correct spline type to create.
	if (type == FUDaeSplineType::UNKNOWN) type = GetType();
	else if (type != GetType()) return NULL;

	// Create the correctly-type spline
	FCDSpline* newSpline = NULL;
	switch (type)
	{
	case FUDaeSplineType::LINEAR: newSpline = new FCDLinearSpline(GetDocument()); break;
	case FUDaeSplineType::BEZIER: newSpline = new FCDBezierSpline(GetDocument()); break;
	case FUDaeSplineType::NURBS: newSpline = new FCDNURBSSpline(GetDocument()); break;

	case FUDaeSplineType::UNKNOWN:
	default: return NULL;
	}

	splines.push_back(newSpline);
	SetDirtyFlag();
	return newSpline;
}

size_t FCDGeometrySpline::GetTotalCVCount()
{
	size_t count = 0;
	for (size_t i = 0; i < splines.size(); i++)
	{
		count += splines[i]->GetCVCount();
	}
	return count;
}

void FCDGeometrySpline::ConvertBezierToNURBS(FCDNURBSSplineList &toFill)
{
	if (type != FUDaeSplineType::BEZIER)
	{
		return;
	}

	for (size_t i = 0; i < splines.size(); i++)
	{
		FCDBezierSpline* bez = static_cast<FCDBezierSpline*>(splines[i]);
		bez->ToNURBs(toFill);
	}
	SetDirtyFlag();
}

// Read in the <spline> node of the COLLADA document
bool FCDGeometrySpline::LoadFromXML(xmlNode* splineNode)
{
	bool status = true;

	// for each spline
	for (; splineNode != NULL; splineNode = splineNode->next)
	{
		// is it a spline?
		if (!IsEquivalent(splineNode->name, DAE_SPLINE_ELEMENT)) continue;

		// needed extra node
		// TODO. those will be moved to attributes
		xmlNode* extraNode = FindChildByType(splineNode, DAE_EXTRA_ELEMENT);
		if (extraNode == NULL) continue;
		xmlNode* fcolladaNode = FindTechnique(extraNode, DAE_FCOLLADA_PROFILE);
		if (fcolladaNode == NULL) continue;
		xmlNode* typeNode = FindChildByType(fcolladaNode, DAE_TYPE_ATTRIBUTE);
		if (typeNode == NULL) continue;

		// get the spline type
		FUDaeSplineType::Type splineType = FUDaeSplineType::FromString(ReadNodeContentFull(typeNode));

		// The types must be compatible
		if (!SetType(splineType))
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_VARYING_SPLINE_TYPE, splineNode->line);
			return status;
		}

		// Read in the typed spline.
		FCDSpline* spline = AddSpline();
		bool s = spline->LoadFromXML(splineNode);
		if (!s)
		{
			SAFE_RELEASE(spline);
			status = false;
		}
	}
		
	SetDirtyFlag();
	return status;
}

// Write out the <spline> node to the COLLADA XML tree
xmlNode* FCDGeometrySpline::WriteToXML(xmlNode* parentNode) const
{
	// create as many <spline> node as there are splines in the array
	for (size_t i = 0; i < splines.size(); ++i)
	{
		const FCDSpline* colladaSpline = splines[i];
		if (colladaSpline == NULL) continue;

		fm::string parentId = GetParent()->GetDaeId();
		fm::string splineId = FUStringConversion::ToString(i);
		colladaSpline->WriteToXML(parentNode, parentId, splineId);
	}

	return NULL;
}
