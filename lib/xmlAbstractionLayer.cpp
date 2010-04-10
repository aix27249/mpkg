/******************************************************************************
 * MOPSLinux package system: XML abstraction layer
 * $Id: xmlAbstractionLayer.cpp,v 1.3 2007/07/16 14:26:14 i27249 Exp $
 * ***************************************************************************/

#include "xmlAbstractionLayer.h"
XMLNode::XMLNode()
{

}

XMLNode::XMLNode(string node_name)
{

}

int XMLNode::parseFile(string fileName)
{
	// TODO for Andrew: parse in the whole file
}

int XMLNode::writeToFile(string fileName)
{
	// TODO for Andrew: write add contents to a file
}

// Tree access functions

XMLNode XMLNode::getChildNode(string nodeName, unsigned int num)	// Returns node
{
	int index=0;
	for (unsigned int i=0; i<_childNodes.size(); i++)
	{
		if (_childNodes[i].getName()==nodeName)
		{
			if (index==num) return _childNodes[i];
			index++;
		}

	}
	fprintf(stderr, "Error: no child node named %s with num %d was found, returning empty node\n",nodeName.c_str(), num);
	XMLNode ret;
	return ret;
}

string XMLNode::getText()						// Returns the text of node
{
	return _text;
}
string XMLNode::getAttributeName(int num)				// Returns num's attribute name (or empty string if it doesn't exist)
{
	if (_attributes.size()>num) return _attributes[num].name;
	else
	{
		fprintf(stderr, "%s Error: no such attribute with num %d, returning empty string\n",__func__, num);
		return "";
	}
}
string XMLNode::getAttributeValue(int num)				// Returns num's attribute value (...)
{
	if (_attributes.size()>num) return _attributes[num].value;
	else
	{
		fprintf(stderr, "%s Error: no such attribute with num %d, returning empty string\n",__func__,num);
		return "";
	}
}
string XMLNode::getAttributeValue(string attributeName)			// Returns the value of attribute named attributeName
{
	for (unsigned int i=0; i<_attributes.size(); i++)
	{
		if (_attributes[i].name==attributeName) return _attributes[i].value;
	}
	fprintf(stderr, "%s Error: no such attribute with name %d, returning empty string\n",__func__,attributeName.c_str());

	return "";

}

string XMLNode::getName()						// Returns the current node name
{
	return _nodeName;
}

int XMLNode::nChildNode(string nodeName)				// Returns the number of child nodes
{
	unsigned int index=0;
	for (unsigned int i=0; i<_childNodes.size(); i++)
	{
		if (_childNodes[i].getName()==nodeName)
		{
			index++;
		}
	}
	return index;
}
int XMLNode::nAttribute(string attributeName)				// Returns the number of attributes
{
	unsigned int index=0;
	for (unsigned int i=0; i<_attributes.size(); i++)
	{
		if (_attributes[i].name==attributeName)
		{
			index++;
		}
	}
	return index;

}
int XMLNode::addChild(XMLNode childNode)				// Add the child tree
{
	_childNodes.push_back(childNode);
	return 0;
}
int XMLNode::addChild(string childNodeName)				// Add the child node
{
	XMLNode new_node;
	new_node.setName(childNodeName);
	_childNodes.push_back(new_node);
}
int XMLNode::addText(string text)					// Add/set the text to current node
{
	_text=text;
}
int XMLNode::addAttribute(string attributeName, string attributeValue)	// Add/set the value attributeValue to attibute named attributeName
{
	// Check if such attribute is already there
	for (unsigned int i=0; i<_attributes.size(); i++)
	{
		if (_attributes[i].name==attributeName)
		{
			_attributes[i].value=attributeValue;
		}
	}
	_attributes.push_back(XMLAttribute(attributeName, attributeValue));

}

int XMLNode::setName(string name)
{
	_nodeName=name;
}

XMLAttribute::XMLAttribute(){}
XMLAttribute::XMLAttribute(string _name, string _value) { name=_name; value=_value; }
XMLAttribute::~XMLAttribute(){}
