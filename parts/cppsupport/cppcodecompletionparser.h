/*
 * file     : cppcodecompletionparser.h
 * begin    : 2001
 * copyright: (c) by daniel engelschalt
 * email    : daniel.engelschalt@htw-dresden.de
 * license  : gpl version >= 2
 *
 */

#ifndef _CPPCCPARSER_H
#define _CPPCCPARSER_H

#include <qstring.h>
#include <qlist.h>
#include <qstack.h>
#include <FlexLexer.h>
#include <fstream.h>
#include <strstream.h>

#include "tokenizer-cc.h"
#include "parsedvariable.h"

/***
 * CppParser : class that parses a .cpp file and stores
 *             all found variables in a QList
 */
class CppCCParser {
public:
    // public methods
    /**
    * empty constructor
    */
    CppCCParser( );

    /**
    * empty copy constructor
    */
    CppCCParser( CppCCParser& cp ){ cerr << "EE: CppCCParser copy constructor called!" << endl;};

    /**
    * empty destructor
    */
    ~CppCCParser( );

    /**
    * parse: the file we'll have to parse
    * @param file to parse
    * @param line until to parse
    */
    bool parse( const QString& file = QString::null, int iCCLine_ = 9999999 );

    /**
    * parse: the string we'll have to parse
    * @param string to parse
    * @param line until to parse
    */
    bool parse( const QString* file = 0, int iCCLine_ = 9999999 );

public:
    // public variables
    /** the place where we store what we found */
    QList<CParsedVariable> varList;

private:
    // private class
    /**
    * this is our read lexem object
    */
    class CParsedLexem {
    public:
        /**
        * constructor
        * @param aType : type of lexem
        * @param aText : text of lexem
        */
        CParsedLexem( int aType, const char* aText ){ type = aType; text = aText; };

	/**
        * standard destructor
        */
        ~CParsedLexem( ){ };

	/**
        * copy constructor
        * @param cp : CParsedLexem
        */
        CParsedLexem( CParsedLexem& cp ){ text = cp.text; type = cp.type; };
    public:
        QString text;
        int type;
    };

private:
    // private attributes
    /** pointer to lexer object */
    yyFlexLexer* lexer;

    /** the lexem (id ) */
    int lexem;

    /** the current file to parse */
    QString currentFile;

    /** a stack for lexem data */
    QStack<CParsedLexem> lexemStack;

    /** LineNo until we parse */
    int iCCLine;

private:
    // private methods
    /**
    * wrapper method called from public parse
    */
    void parseObject( istream& object );

    /**
    * top level parsing
    */
    void parseTopLevel( );

    /**
    * function head parsing
    */
    void parseFunctionHead( );

    /**
    * function body parsing
    */
    void parseFuntionBody( );

    /**
    * function body parsing
    */
    void parseFunctionBody( );

    /**
    * print out what we found
    */
    void debugPrint( );

    /**
    * skip a whole block
    */
    void skipBlock( );

    /**
    * skip a whole command
    */
    void skipCommand( );

    /**
    * skip until a semicolon
    */
    void skipToSemicolon( );

    /**
    * skip to this lexem
    */
    void skipToLexem( const char c );


    /** standard lexem functions */
    /**
    * get next lexem
    */
    void getNextLexem( ){ lexem = lexer->yylex( ); };

    /**
    * get text of lexem
    * @return returns a const char*
    */
    const char* getText( ){ return lexer->YYText( ); };

    /**
    * get linenumber of lexem
    * @return returns an int
    */
    int getLineNo( ){ return lexer->lineno( ) - 1; };

};

#endif
