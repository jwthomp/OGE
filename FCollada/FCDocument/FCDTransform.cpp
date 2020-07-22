/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTransform.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUStringConversion.h"

//
// FCDTransform
//

ImplementObjectType(FCDTransform);

FCDTransform::FCDTransform(FCDocument* document, FCDSceneNode* _parent)
:	FCDObject(document), parent(_parent)
{}

FCDTransform::~FCDTransform()
{
	parent = NULL;
}

void FCDTransform::SetTransformsDirtyFlag()
{
	if (parent != NULL) parent->SetTransformsDirtyFlag();
}

bool FCDTransform::IsInverse(const FCDTransform* UNUSED(transform)) const
{
	return false;
}

void FCDTransform::WriteTransformToXML(xmlNode* transformNode) const
{
	if (!sid.empty())
	{
		fm::string& _sid = const_cast<fm::string&>(sid);
		FUDaeWriter::AddNodeSid(transformNode, _sid);
	}
}

void FCDTransform::SetSubId(const fm::string& subId)
{
	sid = subId; 
	SetDirtyFlag();
}

//
// FCDTTranslation
//

ImplementObjectType(FCDTTranslation);

FCDTTranslation::FCDTTranslation(FCDocument* document, FCDSceneNode* parent)
:	FCDTransform(document, parent)
,	translation(FMVector3::Zero)
{
}

FCDTTranslation::~FCDTTranslation() {}

FCDTransform* FCDTTranslation::Clone(FCDTransform* _clone) const
{
	FCDTTranslation* clone = NULL;
	if (_clone == NULL) clone = new FCDTTranslation(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()));
	else if (!_clone->HasType(FCDTTranslation::GetClassType())) return _clone;
	else clone = (FCDTTranslation*) _clone;

	clone->translation = translation;
	return clone;
}

// Parse a <translate> node from the COLLADA document
bool FCDTTranslation::LoadFromXML(xmlNode* node)
{
	const char* content = FUDaeParser::ReadNodeContentDirect(node);
	FloatList factors;
	factors.reserve(3);
	FUStringConversion::ToFloatList(content, factors);
	if (factors.size() != 3) return false;
	
	translation.Set(factors[0], factors[1], factors[2]);
	FCDAnimatedPoint3::Create(GetDocument(), node, &translation);
	
	SetDirtyFlag();
	return true;
}

// Write the <translate> node to the COLLADA XML document
xmlNode* FCDTTranslation::WriteToXML(xmlNode* parentNode) const
{
	fm::string content = FUStringConversion::ToString(translation);
	xmlNode* transformNode = FUDaeWriter::AddChild(parentNode, DAE_TRANSLATE_ELEMENT);
	FUXmlWriter::AddContentUnprocessed(transformNode, content);
	WriteTransformToXML(transformNode);
	GetDocument()->WriteAnimatedValueToXML(&translation.x, transformNode, !GetSubId().empty() ? GetSubId().c_str() : "translation");
	return transformNode;
}

FMMatrix44 FCDTTranslation::ToMatrix() const
{
	return FMMatrix44::TranslationMatrix(translation);
}

const FCDAnimated* FCDTTranslation::GetAnimated() const
{
	// Returns a FCDAnimatedPoint3
	return GetDocument()->FindAnimatedValue(&translation.x);
}

bool FCDTTranslation::IsAnimated() const
{
	// Only need to check against the first value, because we created a FCDAnimatedPoint3
	return GetDocument()->IsValueAnimated(&translation.x);
}

bool FCDTTranslation::IsInverse(const FCDTransform* transform) const
{
	return transform->GetType() == FCDTransform::TRANSLATION
		&& IsEquivalent(-1.0f * translation, ((const FCDTTranslation*)transform)->translation);
}

//
// FCDTRotation
//

ImplementObjectType(FCDTRotation);

FCDTRotation::FCDTRotation(FCDocument* document, FCDSceneNode* parent)
:	FCDTransform(document, parent)
,	angle(0.0f), axis(FMVector3::XAxis)
{
}

FCDTRotation::~FCDTRotation() {}

FCDTransform* FCDTRotation::Clone(FCDTransform* _clone) const
{
	FCDTRotation* clone = NULL;
	if (_clone == NULL) clone = new FCDTRotation(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()));
	else if (!_clone->HasType(FCDTRotation::GetClassType())) return _clone;
	else clone = (FCDTRotation*) _clone;

	clone->angle = angle;
	clone->axis = axis;
	return clone;
}
// Parse a <rotate> node from the COLLADA document
bool FCDTRotation::LoadFromXML(xmlNode* node)
{
	const char* content = FUDaeParser::ReadNodeContentDirect(node);
	FloatList factors;
	factors.reserve(4);
	FUStringConversion::ToFloatList(content, factors);
	if (factors.size() != 4) return false;

	axis.x = factors[0]; axis.y = factors[1]; axis.z = factors[2];
	angle = factors[3];
	FCDAnimatedAngleAxis::Create(GetDocument(), node, &axis, &angle);

	SetDirtyFlag();
	return true;
}

// Write the <rotate> node to the COLLADA XML document
xmlNode* FCDTRotation::WriteToXML(xmlNode* parent) const
{
	globalSBuilder.clear();
	FUStringConversion::ToString(globalSBuilder, axis); globalSBuilder += ' '; globalSBuilder += angle;
	xmlNode* transformNode = FUDaeWriter::AddChild(parent, DAE_ROTATE_ELEMENT);
	FUXmlWriter::AddContentUnprocessed(transformNode, globalSBuilder);
	WriteTransformToXML(transformNode);
	if (!GetDocument()->WriteAnimatedValueToXML(&axis.x, transformNode, !GetSubId().empty() ? GetSubId().c_str() : "rotation"))
	{
		// If the AngleAxis animated was now used, we should check for the angle by itself.
		GetDocument()->WriteAnimatedValueToXML(&angle, transformNode, !GetSubId().empty() ? GetSubId().c_str() : "rotation");
	}
	return transformNode;
}

FMMatrix44 FCDTRotation::ToMatrix() const
{
	return FMMatrix44::AxisRotationMatrix(axis, FMath::DegToRad(angle));
}

const FCDAnimated* FCDTRotation::GetAnimated() const
{
	// Returns a FCDAnimatedAngleAxis or a FCDAnimatedAngle
	return GetDocument()->FindAnimatedValue(&angle);
}

bool FCDTRotation::IsAnimated() const
{
	// For the axis, we only need to check against the first value, because we created a FCDAnimatedAngleAxis
	return GetDocument()->IsValueAnimated(&angle);
}

bool FCDTRotation::IsInverse(const FCDTransform* transform) const
{
	return transform->GetType() == FCDTransform::ROTATION
		&& IsEquivalent(axis, ((const FCDTRotation*)transform)->axis)
		&& IsEquivalent(-angle, ((const FCDTRotation*)transform)->angle);
}

//
// FCDTScale
//

ImplementObjectType(FCDTScale);

FCDTScale::FCDTScale(FCDocument* document, FCDSceneNode* parent)
:	FCDTransform(document, parent)
,	scale(FMVector3::One)
{
}

FCDTScale::~FCDTScale() {}

FCDTransform* FCDTScale::Clone(FCDTransform* _clone) const
{
	FCDTScale* clone = NULL;
	if (_clone == NULL) clone = new FCDTScale(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()));
	else if (!_clone->HasType(FCDTScale::GetClassType())) return _clone;
	else clone = (FCDTScale*) _clone;

	clone->scale = scale;
	return clone;
}

// Parse a <scale> node from the COLLADA document
bool FCDTScale::LoadFromXML(xmlNode* node)
{
	const char* content = FUDaeParser::ReadNodeContentDirect(node);
	FloatList factors;
	factors.reserve(3);
	FUStringConversion::ToFloatList(content, factors);
	if (factors.size() != 3) return false;

	scale.x = factors[0]; scale.y = factors[1]; scale.z = factors[2];

	// Register the animated values
	FCDAnimatedPoint3::Create(GetDocument(), node, &scale);

	SetDirtyFlag();
	return true;
}

// Write the <scale> node to the COLLADA XML document
xmlNode* FCDTScale::WriteToXML(xmlNode* parentNode) const
{
	fm::string content = FUStringConversion::ToString(scale);
	xmlNode* transformNode = FUDaeWriter::AddChild(parentNode, DAE_SCALE_ELEMENT);
	FUXmlWriter::AddContentUnprocessed(transformNode, content);
	WriteTransformToXML(transformNode);
	GetDocument()->WriteAnimatedValueToXML(&scale.x, transformNode, !GetSubId().empty() ? GetSubId().c_str() : "scale");
	return transformNode;
}

FMMatrix44 FCDTScale::ToMatrix() const
{
	return FMMatrix44::ScaleMatrix(scale);
}

const FCDAnimated* FCDTScale::GetAnimated() const
{
	// Returns a FCDAnimatedPoint3
	return GetDocument()->FindAnimatedValue(&scale.x);
}

bool FCDTScale::IsAnimated() const
{
	// We only need to check against the first value, because we created a FCDAnimatedPoint3
	return GetDocument()->IsValueAnimated(&scale.x);
}

//
// FCDTMatrix
//

ImplementObjectType(FCDTMatrix);

FCDTMatrix::FCDTMatrix(FCDocument* document, FCDSceneNode* parent)
:	FCDTransform(document, parent)
,	transform(FMMatrix44::Identity)
{
}

FCDTMatrix::~FCDTMatrix() {}

FCDTransform* FCDTMatrix::Clone(FCDTransform* _clone) const
{
	FCDTMatrix* clone = NULL;
	if (_clone == NULL) clone = new FCDTMatrix(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()));
	else if (!_clone->HasType(FCDTMatrix::GetClassType())) return _clone;
	else clone = (FCDTMatrix*) _clone;

	clone->transform = transform;
	return clone;
}
// Parse a <matrix> node from the COLLADA document
bool FCDTMatrix::LoadFromXML(xmlNode* node)
{
	const char* content = FUDaeParser::ReadNodeContentDirect(node);
	FUStringConversion::ToMatrix(&content, transform);

	// Register the matrix in the animation system for transform animations
	FCDAnimatedMatrix::Create(GetDocument(), node, &transform);

	SetDirtyFlag();
	return true;
}

// Write the <matrix> node to the COLLADA XML document
xmlNode* FCDTMatrix::WriteToXML(xmlNode* parentNode) const
{
	fm::string content = FUStringConversion::ToString(transform);
	xmlNode* transformNode = FUDaeWriter::AddChild(parentNode, DAE_MATRIX_ELEMENT, content);
	WriteTransformToXML(transformNode);
	GetDocument()->WriteAnimatedValueToXML(&transform[0][0], transformNode, !GetSubId().empty() ? GetSubId().c_str() : "transform");
	return transformNode;
}

const FCDAnimated* FCDTMatrix::GetAnimated() const
{
	// Returns a FCDAnimatedMatrix
	return GetDocument()->FindAnimatedValue(&transform[0][0]);
}

bool FCDTMatrix::IsAnimated() const
{
	// We only need to check against the first value, because we created a FCDAnimatedMatrix
	return GetDocument()->IsValueAnimated(&transform[0][0]);
}

//
// FCDTLookAt
//

ImplementObjectType(FCDTLookAt);

FCDTLookAt::FCDTLookAt(FCDocument* document, FCDSceneNode* parent)
:	FCDTransform(document, parent)
,	position(FMVector3::Zero), target(0.0f, 0.0f, -1.0f), up(FMVector3::YAxis)
{
}

FCDTLookAt::~FCDTLookAt() {}

FCDTransform* FCDTLookAt::Clone(FCDTransform* _clone) const
{
	FCDTLookAt* clone = NULL;
	if (_clone == NULL) clone = new FCDTLookAt(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()));
	else if (!_clone->HasType(FCDTLookAt::GetClassType())) return _clone;
	else clone = (FCDTLookAt*) _clone;

	clone->position = position;
	clone->target = target;
	clone->up = up;
	return clone;
}
// Parse a <lookat> node from the COLLADA document
bool FCDTLookAt::LoadFromXML(xmlNode* lookAtNode)
{
	const char* content = FUDaeParser::ReadNodeContentDirect(lookAtNode);
	FloatList factors;
	factors.reserve(9);
	FUStringConversion::ToFloatList(content, factors);
	if (factors.size() != 9) return false;

	position.Set(factors[0], factors[1], factors[2]);
	target.Set(factors[3], factors[4], factors[5]);
	up.Set(factors[6], factors[7], factors[8]);

	SetDirtyFlag();
	return true;
}

// Write the <lookat> node to the COLLADA XML document
xmlNode* FCDTLookAt::WriteToXML(xmlNode* parentNode) const
{
	globalSBuilder.clear();
	FUStringConversion::ToString(globalSBuilder, position); globalSBuilder.append(' ');
	FUStringConversion::ToString(globalSBuilder, target); globalSBuilder.append(' ');
	FUStringConversion::ToString(globalSBuilder, up);
	xmlNode* transformNode = FUDaeWriter::AddChild(parentNode, DAE_LOOKAT_ELEMENT, globalSBuilder);
	WriteTransformToXML(transformNode);
	return transformNode;
}

FMMatrix44 FCDTLookAt::ToMatrix() const
{
	return FMMatrix44::LookAtMatrix(position, target, up);
}

const FCDAnimated* FCDTLookAt::GetAnimated() const
{
	// Does not support animations
	return NULL;
}

bool FCDTLookAt::IsAnimated() const
{
	// Does not support animations
	return false;
}

//
// FCDTSkew
//

ImplementObjectType(FCDTSkew);

FCDTSkew::FCDTSkew(FCDocument* document, FCDSceneNode* parent)
:	FCDTransform(document, parent)
,	rotateAxis(FMVector3::XAxis), aroundAxis(FMVector3::YAxis), angle(0.0f)
{
}

FCDTSkew::~FCDTSkew()
{
}

FCDTransform* FCDTSkew::Clone(FCDTransform* _clone) const
{
	FCDTSkew* clone = NULL;
	if (_clone == NULL) clone = new FCDTSkew(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()));
	else if (!_clone->HasType(FCDTSkew::GetClassType())) return _clone;
	else clone = (FCDTSkew*) _clone;

	clone->rotateAxis = rotateAxis;
	clone->aroundAxis = aroundAxis;
	clone->angle = angle;
	return clone;
}

// Parse a <skew> element from the COLLADA document
bool FCDTSkew::LoadFromXML(xmlNode* skewNode)
{
	const char* content = FUDaeParser::ReadNodeContentDirect(skewNode);
	FloatList factors;
	factors.reserve(7);
	FUStringConversion::ToFloatList(content, factors);
	if (factors.size() != 7) return false;

	angle = factors[0];
	rotateAxis.x = factors[1]; rotateAxis.y = factors[2]; rotateAxis.z = factors[3];
	aroundAxis.x = factors[4]; aroundAxis.y = factors[5]; aroundAxis.z = factors[6];

	// Check and pre-process the axises
	if (IsEquivalent(rotateAxis, FMVector3::Origin) || IsEquivalent(aroundAxis, FMVector3::Origin)) return false;
	rotateAxis = rotateAxis.Normalize();
	aroundAxis = aroundAxis.Normalize();

	// Register the animated values
	FCDAnimatedAngle::Create(GetDocument(), skewNode, &angle);

	SetDirtyFlag();
	return true;
}

// Write the <lookat> node to the COLLADA XML document
xmlNode* FCDTSkew::WriteToXML(xmlNode* parentNode) const
{
	globalSBuilder.clear();
	globalSBuilder.set(angle); globalSBuilder += ' ';
	FUStringConversion::ToString(globalSBuilder, rotateAxis); globalSBuilder += ' ';
	FUStringConversion::ToString(globalSBuilder, aroundAxis);
	xmlNode* transformNode = FUDaeWriter::AddChild(parentNode, DAE_SKEW_ELEMENT);
	FUXmlWriter::AddContentUnprocessed(transformNode, globalSBuilder);
	WriteTransformToXML(transformNode);
	GetDocument()->WriteAnimatedValueToXML(&angle, transformNode, !GetSubId().empty() ? GetSubId().c_str() : "skew");
	return transformNode;
}

FMMatrix44 FCDTSkew::ToMatrix() const
{
	float v[4][4];

	float s = tanf(FMath::DegToRad(angle));

	for (int row = 0; row < 3; ++row)
	{
		for (int col = 0; col < 3; ++col)
		{
			v[col][row] = ((row == col) ? 1.0f : 0.0f) + s * rotateAxis[col] * aroundAxis[row];
		}
	}

	v[0][3] = v[1][3] = v[2][3] = 0.0f;
	v[3][0] = v[3][1] = v[3][2] = 0.0f;
	v[3][3] = 1.0f;

	return FMMatrix44((float*) v);
}

const FCDAnimated* FCDTSkew::GetAnimated() const
{
	return GetDocument()->FindAnimatedValue(&angle);
}

bool FCDTSkew::IsAnimated() const
{
	// Only angle may be animated for now.
	return GetDocument()->IsValueAnimated(&angle);
}

// Creates a new COLLADA transform, given a transform type.
FCDTransform* FCDTFactory::CreateTransform(FCDocument* document, FCDSceneNode* parent, FCDTransform::Type type)
{
	switch (type)
	{
	case FCDTransform::TRANSLATION: return new FCDTTranslation(document, parent);
	case FCDTransform::ROTATION: return new FCDTRotation(document, parent);
	case FCDTransform::SCALE: return new FCDTScale(document, parent);
	case FCDTransform::SKEW: return new FCDTSkew(document, parent);
	case FCDTransform::MATRIX: return new FCDTMatrix(document, parent);
	case FCDTransform::LOOKAT: return new FCDTLookAt(document, parent);
	default: return NULL;
	}
}

// Imports a COLLADA transform, given an XML tree node.
FCDTransform* FCDTFactory::CreateTransform(FCDocument* document, FCDSceneNode* parent, xmlNode* node)
{
	FCDTransform* transform = NULL;
	if (IsEquivalent(node->name, DAE_ROTATE_ELEMENT)) transform = new FCDTRotation(document, parent);
	else if (IsEquivalent(node->name, DAE_TRANSLATE_ELEMENT)) transform = new FCDTTranslation(document, parent);
	else if (IsEquivalent(node->name, DAE_SCALE_ELEMENT)) transform = new FCDTScale(document, parent);
	else if (IsEquivalent(node->name, DAE_SKEW_ELEMENT)) transform = new FCDTSkew(document, parent);
	else if (IsEquivalent(node->name, DAE_MATRIX_ELEMENT)) transform = new FCDTMatrix(document, parent);
	else if (IsEquivalent(node->name, DAE_LOOKAT_ELEMENT)) transform = new FCDTLookAt(document, parent);
	return transform;
}
