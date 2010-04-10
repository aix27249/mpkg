/******************************************************************************
 * MOPSLinux package system: XML abstraction layer
 * $Id: xmlAbstractionLayer.h,v 1.4 2007/07/19 10:29:44 i27249 Exp $
 * ***************************************************************************/
#ifndef MPKG_XML_ABS_LAYER
#define MPKG_XML_ABS_LAYER
#include <string>
#include <vector>
using namespace std;

class XMLAttribute
{
	public:
		XMLAttribute();
		XMLAttribute(string _name, string _value);
		~XMLAttribute();
	string name;
	string value;
};

class XMLNode
{
	public:
		// Constructors
		XMLNode();
		XMLNode(string nodeName);
		int parseFile(string fileName);
		int writeToFile(string fileName);

		// Tree access functions
		XMLNode getChildNode(string nodeName, unsigned int num=0);	// Returns node
		string getText();						// Returns the text of node
		string getAttributeName(int num);				// Returns num's attribute name (or empty string if it doesn't exist)
		string getAttributeValue(int num);				// Returns num's attribute value (...)
		string getAttributeValue(string attributeName);			// Returns the value of attribute named attributeName
		string getName();						// Returns the current node name

		int nChildNode(string nodeName);				// Returns the number of child nodes
		int nAttribute(string attributeName);				// Returns the number of attributes

		int addChild(XMLNode childNode);				// Add the child tree
		int addChild(string childNodeName);				// Add the child node
		int addText(string text);					// Add/set the text to current node
		int addAttribute(string attributeName, string attributeValue);	// Add/set the value attributeValue to attibute named attributeName
		int setName(string name);					// Sets the current node name

	private:
		vector<XMLNode> _childNodes;
		string _text;
		vector<XMLAttribute> _attributes;
		string _nodeName;
};

#endif
