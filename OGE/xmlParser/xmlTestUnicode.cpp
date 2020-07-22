/**
 ****************************************************************************
 * <P> XML.c - XML parser test example - wchar_t* version </P>
 *
 * @version     V2.27
 * @author      Frank Vanden Berghen
 *
 * BSD license:
 * Copyright (c) 2002, Frank Vanden Berghen
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Frank Vanden Berghen nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************
 */
#ifdef WIN32
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include "xmlParser.h"
ToXMLStringTool tx,tx2;

int main(int argc, char **argv)
{
    /*********************************************************************************
     *                                                                               *
     *  Example 1: Basic operations to parse and collect data from a XML file        *
     *                                                                               *
     *********************************************************************************/

    // this open and parse the XML file:
    XMLNode xMainNode=XMLNode::openFileHelper(L"PMMLModel.xml",L"PMML");

    // this prints "RANK For <you>":
    XMLNode xNode=xMainNode.getChildNode(L"Header");
    printf("Application Name is: '%S'\n", xNode.getChildNode(L"Application").getAttribute(L"name"));

    // this prints "Hello World!"
    printf("Text inside Header tag is :'%S'\n", xNode.getText());

    // this gets the number of "NumericPredictor" tags:
    xNode=xMainNode.getChildNode(L"RegressionModel").getChildNode(L"RegressionTable");
    int n=xNode.nChildNode(L"NumericPredictor");

    // this prints the "coefficient" value for all the "NumericPredictor" tags:
    int i,myIterator=0;
    for (i=0; i<n; i++)
    printf("coeff %i=%S\n",i+1,xNode.getChildNode(L"NumericPredictor",&myIterator).getAttribute(L"coefficient"));

    // this create a file named "testUnicode.xml" based on the content of the first "Extension" tag of the XML file:
    xMainNode.getChildNode(L"Extension").writeToFile(L"testUnicode.xml");

    printf("The content of the clear tag is:%S",xMainNode.getChildNode(L"html_page").getClear().lpszValue);

    /****************************************************************************************
     *                                                                                      *
     *  Example 2: memory management: when to use the 'stringDup' and the 'free' functions  *
     *                                                                                      *
     ****************************************************************************************/

    // compare these 4 lines ...
    wchar_t *t=stringDup(xMainNode.getAttribute(L"version"));       // get version number
    xMainNode=XMLNode::emptyXMLNode;                          // free from memory the top of the xml Tree
    printf("PMML Version :%S\n",t);                           // print version number
    free(t);                                                  // free version number

    // ... with the following 3 lines (currently commented, because of error):
    //  t=xMainNode.getAttribute(L"version");      // get version number (note that there is no 'stringDup')
    //  xMainNode=XMLNode::emptyXMLNode;           // free from memory the top of the xml Tree AND the version number inside 't' var
    //  printf("PMML Version :%S\n",t);            // since the version number in 't' has been free'd this will not work

    /**********************************************************
     *                                                        *
     *  Example 3: constructing & updating a tree of XMLNode  *
     *                                                        *
     **********************************************************/

    // We create in memory from scratch the following XML structure:
    //  <?xml version="1.0"?>
    //      <body color="#FFFFFF"> Hello "universe". </body>
    // ... and we transform it into a standard C string that is printed on screen.
    xMainNode=XMLNode::createXMLTopNode(L"xml",TRUE);
    xMainNode.addAttribute(L"version",L"1.0");
    xNode=xMainNode.addChild(L"body");
    xNode.addText(L"Hello \"univ\"!");
    xNode.deleteText();
    xNode.addText(L"Hello \"universe\"!");
    xNode.addAttribute(L"color",L"#wrongcolor");
    xNode.updateAttribute(L"#FFFFFF",NULL,L"color");

    t=xMainNode.createXMLString(false);
    printf("XMLString created from scratch:\n%S",t);
    free(t);

    // we delete some parts:
    xNode.deleteAttribute(L"color");
    t=xMainNode.createXMLString(false);
    printf("\nWith the \"color\" attribute deleted:\n%S\n\n",t);
    free(t);

    /*********************************************************************************************************
     *                                                                                                       *
     *  Example 4: by default, the XML parser is "forgiving" with respect to errors inside XML strings&files *
     *                                                                                                       *
     *********************************************************************************************************/

    // By default, the XML parser is "forgiving":
    // (You can de-activate this behavior: see the header of xmlParser.cpp file)
    wchar_t *t2=(wchar_t*)L"<a><b>some text</b><b>other text    </a>";
    XMLResults xe;
    xMainNode=XMLNode::parseString(t2,NULL,&xe);
    t=xMainNode.createXMLString(false);
    printf("The following XML: %S\n  ...is parsed as: %S\nwith the following info: '%S'\n",t2,t?t:L"(null)",XMLNode::getError(xe.error));
    free(t);

    /*******************************************************
     *                                                     *
     *  Example 5: deleting a part of the tree of XMLNode  *
     *                                                     *
     *******************************************************/

    // this deletes the "<b>other text</b>" subtree part:
    xMainNode.getChildNode(L"b",1).deleteNodeContent();

    // To perform the same "delete" as above, we can also do:
    // xNode=xMainNode.getChildNode(L"a").getChildNode(L"b",1); xNode.deleteNodeContent(); xNode=XMLNode::emptyXMLNode;
    // If you forget the last part of the delete ("xNode=XMLNode::emptyXMLNode"), then the XMLNode will NOT be deleted:
    // As long as there exists a reference to an XMLNode, the smartPointer mechanism prevent the node to be deleted.

    t=xMainNode.createXMLString(false);
    printf("\n...with the wrong node deleted: %S\n",t);
    free(t);

    /************************************************************************************************************
    *                                                                                                          *
    *  Example 5: inserting (and moving) a new XMLNode in the middle of an already existing XMLNode structure  *
    *                                                                                                          *
    ************************************************************************************************************/

    // This creates a XMLNode 'a' that is "<a><b>some text</b><b>other text</b></a>":
    xMainNode=XMLNode::parseString(t2);
    // This creates a XMLNode 'c' that is "<c>hello</c>":
    xNode=XMLNode::parseString(L"<c>hello</c>");

    xMainNode.addChild(xNode,0);
    t=xMainNode.createXMLString(false);
    printf("\nWe inserted a new node 'c' as the first tag inside 'a':\n       %S",t);
    free(t);

    xMainNode.addChild(xNode,xMainNode.positionOfChildNode(L"b",1));
    t=xMainNode.createXMLString(false);
    printf("\nWe moved the node 'c' at the position of the second 'b' tag:\n       %S\n",t);
    free(t);

    /*******************************************
     *                                         *
     *  Example 6: base 64 encoding/decoding   *
     *                                         *
     *******************************************/

    unsigned char *originalBinaryData=(unsigned char *)"this is binary data.";
    XMLParserBase64Tool b64;
    t=b64.encode(originalBinaryData,21);
    printf(
      "\nTo be able to include any binary data into an xml file, some Base64 conversion"
      "\nfunctions (binary data <--> ascii text) are provided:\n"
      "  original binary data   : %s\n"
      "  encoded as text        : %S\n",originalBinaryData,t);
    printf("  decoded as binary again: %s\n",b64.decode(t));

    /******************************************************
     *                                                    *
     *  Example 7: usage of the "getParentNode()" method  *
     *                                                    *
     ******************************************************/

    printf("\nTwo examples of usage of the \"getParentNode()\" method:\n");
    // In the following two examples, I create a tree of XMLNode based on the string
    // "<a><b>some text</b><b>other text</b></a>". After parsing this string
    // I get a XMLNode that represents the <a> tag. Thereafter I "go down" one
    // level, using getChildNode: I now have a XMLNode that represents the <b> tag.
    // Thereafter I "go up" one level, using getParentNode(): I now have once again
    // a XMLNode that represents the <a> tag. Thereafter, I print the name ('a') of
    // this last XMLNode. The first example below is working as intended (it prints 'a'
    // on the screen). However, the second example below prints "null" because when we
    // did "xMainNode=xMainNode.getChildNode()" we lost all references to the
    // top node and thus it's automatically "garbage collected" (free memory).
    xMainNode=XMLNode::parseString(t2);     xNode=xMainNode.getChildNode();         xNode=xNode.getParentNode(); t=(wchar_t*)    xNode.getName(); printf(" Ex1: Name of top node; '%S'\n",t?t:L"null");
    xMainNode=XMLNode::parseString(t2); xMainNode=xMainNode.getChildNode(); xMainNode=xMainNode.getParentNode(); t=(wchar_t*)xMainNode.getName(); printf(" Ex2: Name of top node; '%S'\n",t?t:L"null");


    /******************************************************
     *                                                    *
     *  Example 8: usage of the ToXMLStringTool class     *
     *                                                    *
     ******************************************************/

    // For performance reason it's sometime better to use the old-style "fwprintf"
    // function to create a XML file directly without constructing first
    // a XMLNode structure. In such case, the ToXMLStringTool class comes in handy.

    const wchar_t *t3=L"Hello to the <\"World\">";
    printf("\nToXMLStringTool demo: Original String: %S\n"
        "                      Encoded in XML : %S\n",t3,ToXMLStringTool().toXML(t3));

    // If you use several time (in different "fwprintf") the same ToXMLStringTool
    // object, then the memory allocation (needed to create the output
    // buffer) will be performed only once. This is very efficient, very fast.
    // Usually, I create a global instance of the ToXMLStringTool class named "tx" (see
    // line 42 of this file) and then I use "tx" everywhere. For example:
    const wchar_t *t4=L"I say 'pick-a-boo'!";
    printf("Global ToXMLStringTool tx: %S\n",tx.toXML(t4));
    printf("Global ToXMLStringTool tx: %S\n",tx.toXML(t3));

    // However you must be careful because sometime the output buffer might be
    // erased before being printed. The next example is not working:
    printf("Error using ToXMLStringTool: %S\n"
        "                             %S\n",tx.toXML(t4),tx.toXML(t3));

    // However, this is working fine:
    printf("Correct usage of ToXMLStringTool: %S\n"
        "                                  %S\n",tx.toXML(t4),tx2.toXML(t3));

    // Using the "ToXMLStringTool class" and the "fwprintf function" is THE most efficient
    // way to produce VERY large XML documents VERY fast.

    return 0;
}
