/* Definition of member functions of class Parser */


#include "../headers/Parser.hpp"

#include <cstdlib>
#ifdef DEBUG
# include <iostream>
#endif

using namespace std;

Parser::Parser() :
  m_currentToken(),
  m_scanner(nullptr),
  m_errorReporter(nullptr),
  m_maxErrors(5),
  m_maxRuleIterations(1000),
  m_nTokensProcessed(0),
  m_semanticChecker(),
  m_function(),
  m_variables()
{
}

void Parser::setErrorReporter(ErrorReporter* errorReporter)
{
  m_errorReporter = errorReporter;
  m_maxErrors = m_errorReporter->getMaxErrors();
}

void Parser::setScanner(Scanner* scanner)
{
  m_scanner = scanner;
}

void Parser::parse()
{
#ifdef DEBUG
  cout << "::: Scanning :::::::::::::::::::::::::::::::::::::::::::::" << endl;
#endif
  m_scanner->scan();
#ifdef DEBUG
  cout << "Total tokens: " << m_scanner->getMaxTokens() << endl;
#endif

  if (m_errorReporter->getErrors() >= m_maxErrors)
    return; 

#ifdef DEBUG
  cout << "::: Parsing ::::::::::::::::::::::::::::::::::::::::::::::" << endl;
#endif
  m_semanticChecker.setErrorReporter(m_errorReporter);
  program();

  if (!m_semanticChecker.isMainPresent())
  {
    m_errorReporter->writeError("no se declaro una funcion principal");
  }

#ifdef DEBUG
  cout << "expected tokens: " << m_scanner->getTokensProcessed() <<
      ", got: " << m_scanner->getMaxTokens() << endl;
#endif
  if (m_scanner->getTokensProcessed() != m_scanner->getMaxTokens())
  {
    m_errorReporter->writeError(
        "codigo incompleto, cantidad de tokens incongruente");
  }

#ifdef DEBUG
  m_semanticChecker.printSymbolsTable();
  m_semanticChecker.printTypesStack();
#endif
}

void Parser::andOperation()
{
#ifdef DEBUG
  cout << "::: entering andoperation()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  unsigned int iterations = 0;
  do
  {
    notOperation();

    ++iterations;

    if (m_currentToken.getLexeme().compare("&&") == 0)
    {
      m_semanticChecker.pushOperator(OPERATOR_LOGIC);
    }
  } while (m_currentToken.getLexeme().compare("&&") == 0 &&
           iterations < m_maxRuleIterations);

#ifdef DEBUG
  cout << "::: exit andOperation()" << endl;
#endif
}

void Parser::argumentsList()
{
#ifdef DEBUG
  cout << "::: entering argumentsList()" << endl;
#endif

  bool isOperatorFound = false;
  string argumentTypes;

  unsigned int iterations = 0;
  do
  {
    isOperatorFound = false;
    expression();

    advanceToken();
    if (m_currentToken.getLexeme().compare(",") == 0)
    {
      isOperatorFound = true;
      advanceToken();
    }

    ++iterations;
  } while (isOperatorFound && iterations < m_maxRuleIterations);

  m_scanner->moveBackwards();

#ifdef DEBUG
  cout << "::: exit argumentsList()" << endl;
#endif
}

void Parser::assign()
{
#ifdef DEBUG
  cout << "::: entering assign()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  checkToken(TOKEN_ASSIGNOP);
  advanceToken();
  expression();
#ifdef DEBUG
  cout << "::: exit assign()" << endl;
#endif
}

void Parser::block()
{
#ifdef DEBUG
  cout << "::: entering block()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  checkLexeme("{");
  advanceToken();
  if (m_currentToken.getLexeme().compare("}") != 0)
  {
    m_scanner->moveBackwards();
    statements();
  }
  ignoreNewLines();
  checkLexeme("}");
#ifdef DEBUG
  cout << "::: exit block()" << endl;
#endif
}

void Parser::switchStatement()
{
#ifdef DEBUG
  cout << "::: entering caseStatement()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  m_semanticChecker.enterSwitch();
  
  checkLexeme("caso");
  checkToken(TOKEN_IDEN);
  checkLexeme("{");

  do
  {
    ignoreNewLines();
    do
    {
      if (m_currentToken.getLexeme().compare("valor") == 0)
      {
        checkLexeme("valor");
        checkLiteral();
      }
      else if (m_currentToken.getLexeme().compare("defecto") == 0)
      {
        checkLexeme("defecto");
      }
      checkLexeme(":");
      advanceToken();
    } while (m_currentToken.getLexeme().compare("valor") == 0 ||
             m_currentToken.getLexeme().compare("defecto") == 0);

    m_scanner->moveBackwards();
    statements();
  } while (m_currentToken.getLexeme().compare("valor") == 0 ||
           m_currentToken.getLexeme().compare("defecto") == 0);

  checkLexeme("}");

  m_semanticChecker.exitSwitch();
#ifdef DEBUG
  cout << "::: exit caseStatement()" << endl;
#endif
}

void Parser::command()
{
#ifdef DEBUG
  cout << "::: entering command()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

#ifdef DEBUG
  cout << "checking for command first word, got: " <<
      m_currentToken.getLexeme() << endl;
#endif

  if (m_currentToken.getToken() == TOKEN_IDEN)
  {
    if (m_currentToken.getLexeme().compare("con") == 0)
    {
      read();
    }
    else if (m_currentToken.getLexeme().compare("fmt") == 0)
    {
      print();
    }
    else
    {
      // FIXME: this should not need the double forward movement.
      m_scanner->moveForward();
      ++m_nTokensProcessed;
      advanceToken();

      if (m_currentToken.getToken() == TOKEN_ASSIGNOP)
      {
        m_scanner->moveBackwards();

        assign();
      }
      else if (m_currentToken.getLexeme().compare("[") == 0)
      {
        m_scanner->moveBackwards();

        dimension();
        assign();
      }
      else if (m_currentToken.getLexeme().compare("(") == 0)
      {
        m_scanner->moveBackwards();

        functionCall();
      }
      else
      {
        m_errorReporter->writeSyntaxError(":=, ( o [");
      }
    }
  }
  else if (m_currentToken.getLexeme().compare("si") == 0)
  {
    ifStatement();
  }
  else if (m_currentToken.getLexeme().compare("desde") == 0)
  {
    forStatement();
  }
  else if (m_currentToken.getLexeme().compare("caso") == 0)
  {
    switchStatement();
  }
  else if (m_currentToken.getLexeme().compare("regresa") == 0)
  {
    returnExpression();
  }
  else if (m_currentToken.getLexeme().compare("var") == 0)
  {
    variablesDeclaration();
  }
  else if (m_currentToken.getLexeme().compare("interrumpe") == 0)
  {
    advanceToken();

    if (!m_semanticChecker.isInSwitch() && !m_semanticChecker.isInFor())
    {
      TokenLexeme lastToken = m_scanner->getLastToken();
      m_errorReporter->writeErrorWithPosition(
          "interrumpe solo es permitido en ciclo desde o en caso");
    }
  }
  else if (m_currentToken.getLexeme().compare("continua") == 0)
  {
    advanceToken();
    if (!m_semanticChecker.isInFor())
    {
      TokenLexeme lastToken = m_scanner->getLastToken();
      m_errorReporter->writeErrorWithPosition(
            "continua solo es permitido en ciclo desde");
    }
  }
  else
  {
    m_errorReporter->writeErrorWithPosition("inicio de comando invalido");
  }
#ifdef DEBUG
  cout << "::: exit command()" << endl;
#endif
}

void Parser::constantsDeclaration()
{
#ifdef DEBUG
  cout << "::: entering constantsDeclaration()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  checkLexeme("const");

  advanceToken();
  if (m_currentToken.getLexeme().compare("(") == 0)
  {
    unsigned int iterations = 0;
    do
    {
      constant();
      advanceToken();
#ifdef DEBUG
    cout << "::: current lexeme (line " << __LINE__ << "): " <<
        m_currentToken.getLexeme() << endl;
#endif
      ++iterations;
    } while (m_currentToken.getToken() == TOKEN_NEWLINE &&
             iterations < m_maxRuleIterations);
  }
  else
  {
    constant();
  }
#ifdef DEBUG
  cout << "::: exit constantsDeclaration()" << endl;
#endif
}

void Parser::constant()
{
#ifdef DEBUG
  cout << "::: entering constant()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  checkToken(TOKEN_IDEN);

  SymbolData variable;
  variable.setName(m_scanner->getLastToken().getLexeme());
  variable.setLine(m_scanner->getLastToken().getLine());
  variable.setIsConstant(true);
  m_variables.push(variable);

  checkToken(TOKEN_ASSIGNOP);

  checkLiteral();
  resolveVariablesType(
      SymbolData::getLiteralType(m_scanner->getLastToken().getToken()));

#ifdef DEBUG
  cout << "::: exit constant()" << endl;
#endif
}

void Parser::dimension()
{
#ifdef DEBUG
  cout << "::: entering dimension()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  unsigned int iterations = 0;
  size_t dimensions = 0;
  do {
    ++dimensions;
    checkLexeme("[");
    advanceToken();
    expression();

    checkLexeme("]");
    advanceToken();
    if (m_currentToken.getLexeme().compare("[") == 0)
    {
      m_scanner->moveBackwards();
    }

    ++iterations;
  } while (m_currentToken.getLexeme().compare("[") == 0 &&
           iterations < m_maxRuleIterations);
  
  m_variables.front().setDimensions(dimensions);

  m_scanner->moveBackwards();

#ifdef DEBUG
  cout << "::: exit dimension()" << endl;
#endif
}

void Parser::exponent()
{
#ifdef DEBUG
  cout << "::: entering exponent()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  sign();

  if (m_currentToken.getLexeme().compare("^") == 0)
  {
    m_semanticChecker.pushOperator(OPERATOR_ARITH);

    sign();
  }

#ifdef DEBUG
  cout << "::: exit exponent()" << endl;
#endif
}

void Parser::expression()
{
#ifdef DEBUG
  cout << "::: entering expression()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  unsigned int iterations = 0;
  do
  {
    andOperation();

    ++iterations;

    if (m_currentToken.getLexeme().compare("||") == 0)
    {
      m_semanticChecker.pushOperator(OPERATOR_LOGIC);
    }
  } while (m_currentToken.getLexeme().compare("||") == 0 &&
           iterations < m_maxRuleIterations);

  m_scanner->moveBackwards();

#ifdef DEBUG
  cout << "::: exit expression()" << endl;
#endif
}

void Parser::forStatement()
{
#ifdef DEBUG
  cout << "::: entering forStatement()" << endl;
#endif
  m_semanticChecker.enterFor();

  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;
  
  checkLexeme("desde");

  advanceToken();

  if (m_currentToken.getLexeme().compare(";") != 0)
  {
    assign();
    unsigned int iterations = 0;
    while (m_currentToken.getLexeme().compare(",") == 0 &&
           iterations < m_maxRuleIterations)
    {
      assign();
      
      ++iterations;
    }
  }
  else
  {
    m_scanner->moveBackwards();
  }

  checkLexeme(";"); 

  advanceToken();
  if (m_currentToken.getLexeme().compare(";") != 0)
  {
    expression();
  }
  else
  {
    m_scanner->moveBackwards();
  }

  checkLexeme(";");

  advanceToken();
  if (m_currentToken.getLexeme().compare("{") != 0)
  {
    assign();
    unsigned int iterations = 0;
    while (m_currentToken.getLexeme().compare(",") == 0 &&
           iterations < m_maxRuleIterations)
    {
      assign();
      advanceToken();

      ++iterations;
    }
  }
  else
  {
    m_scanner->moveBackwards();
  }

  block();

  m_semanticChecker.exitFor();
#ifdef DEBUG
  cout << "::: exit forStatement()" << endl;
#endif
}

void Parser::functionCall()
{
#ifdef DEBUG
  cout << "::: entering functionCall()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  checkLexeme("(");
  
  advanceToken();
  if (m_currentToken.getToken() == TOKEN_IDEN ||
      m_currentToken.getToken() == TOKEN_STRING ||
      m_currentToken.getToken() == TOKEN_DEC ||
      m_currentToken.getToken() == TOKEN_HEX ||
      m_currentToken.getToken() == TOKEN_OCT ||
      m_currentToken.getToken() == TOKEN_FLOAT)
  {
    argumentsList();
  }
  else
  {
    m_scanner->moveBackwards();
  }

  checkLexeme(")");
  advanceToken();
#ifdef DEBUG
  cout << "::: exit functionCall()" << endl;
#endif
}

void Parser::functionDeclaration()
{
#ifdef DEBUG
  cout << "::: entering functionDeclaration()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  checkLexeme("funcion");
  checkToken(TOKEN_IDEN);

  m_function.setName(m_scanner->getLastToken().getLexeme());
  m_function.setLine(m_scanner->getLastToken().getLine());
  m_function.setType(TYPE_VOID);
  m_function.setIsFunction(true);

  if (m_scanner->getLastToken().getLexeme().compare("principal") == 0)
  {
    m_semanticChecker.setMainPresent(true);
  }

  m_semanticChecker.enterToScope(m_scanner->getLastToken().getLexeme());

  checkLexeme("(");
  parameterList();
  checkLexeme(")");
  returnType();

  m_semanticChecker.declare(m_function);
  m_function.reset();

  block();

  m_semanticChecker.checkReturnRequired();

  m_semanticChecker.exitCurrentScope();
#ifdef DEBUG
  cout << "::: exit functionDeclaration()" << endl;
#endif
}

void Parser::ifStatement()
{
#ifdef DEBUG
  cout << "::: entering ifStatement()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  checkLexeme("si");

  advanceToken();
  expression();
  block();
  ignoreNewLines();

  if (m_currentToken.getLexeme().compare("sino") == 0)
  {
    checkLexeme("sino");
    block();
  }

#ifdef DEBUG
  cout << "::: exit ifStatement()" << endl;
#endif
}

void Parser::import()
{
#ifdef DEBUG
  cout << "::: entering import()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  checkLexeme("importar");

  advanceToken();
  if (m_currentToken.getLexeme().compare("(") == 0)
  {
    unsigned int iterations = 0;
    do
    {
      checkToken(TOKEN_STRING);

      m_semanticChecker.addImport(m_scanner->getLastToken().getLexeme());

      advanceToken();
      
      ++iterations;
    } while (m_currentToken.getToken() == TOKEN_NEWLINE &&
             iterations < m_maxRuleIterations);

#ifdef DEBUG
    cout << "::: current lexeme (line " << __LINE__ << "): " <<
        m_currentToken.getLexeme() << endl;
#endif
  }
  else
  {
    checkToken(TOKEN_STRING);
    checkToken(TOKEN_NEWLINE);
  }
#ifdef DEBUG
  cout << "::: exit import() ::" << endl;
#endif
}

void Parser::multiplication()
{
#ifdef DEBUG
  cout << "::: entering multiplication()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  bool isOperatorFound = false;
  unsigned int iterations = 0;
  do
  {
    isOperatorFound = false;
    exponent();
    if (m_currentToken.getLexeme().compare("*") == 0 ||
        m_currentToken.getLexeme().compare("/") == 0 ||
        m_currentToken.getLexeme().compare("%") == 0)
    {
      isOperatorFound = true;

      m_semanticChecker.pushOperator(OPERATOR_ARITH);

      advanceToken();
    }

    ++iterations;
  } while (isOperatorFound && iterations < m_maxRuleIterations); 
#ifdef DEBUG
  cout << "::: exit multiplication()" << endl;
#endif
}

void Parser::notOperation()
{
#ifdef DEBUG
  cout << "::: entering notOperation()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  if (m_currentToken.getLexeme().compare("!") == 0)
  {
    checkLexeme("!");

    m_semanticChecker.pushOperator(OPERATOR_NOT);

    relationalOperation();
  }
  relationalOperation();
#ifdef DEBUG
  cout << "::: exit notOperation()" << endl;
#endif
}

void Parser::parameterList()
{
#ifdef DEBUG
  cout << "::: entering parameterList()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  unsigned int iterations = 0;
  unsigned int nTypeParameters = 0;
  NativeType_t parameterType = TYPE_INVALID;
  advanceToken();
  while (m_currentToken.getLexeme().compare(")") != 0 &&
         iterations < m_maxRuleIterations)
  {
    nTypeParameters = 0;
    m_scanner->moveBackwards();

    do
    {
      checkToken(TOKEN_IDEN);

      SymbolData variable;
      variable.setName(m_scanner->getLastToken().getLexeme());
      variable.setLine(m_scanner->getLastToken().getLine());
      m_variables.push(variable);

      advanceToken();

      ++nTypeParameters;
      ++iterations;
    } while (m_currentToken.getLexeme().compare(",") == 0 &&
             iterations < m_maxRuleIterations);

    m_scanner->moveBackwards();
    checkNativeDataType();

    parameterType =
        SymbolData::getStringType(m_scanner->getLastToken().getLexeme());
    for (unsigned int i = 0; i < nTypeParameters; ++i)
    {
      m_function.addParameter(parameterType);
    }

    resolveVariablesType(parameterType);

    advanceToken();

    ++iterations;
  }

  m_scanner->moveBackwards();
#ifdef DEBUG
  cout << "::: exit parameterList()" << endl;
#endif
}

void Parser::print()
{
#ifdef DEBUG
    cout << "::: entering print()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  checkLexeme("fmt");

  m_semanticChecker.checkImported("\"fmt\"");

  checkLexeme(".");

  advanceToken();
  if (m_currentToken.getLexeme().compare("Imprime") != 0 &&
      m_currentToken.getLexeme().compare("Imprimenl") != 0)
  {
    m_errorReporter->writeSyntaxError("Imprime o Imprimenl");
  }
  
  checkLexeme("(");
  advanceToken();
  argumentsList();
  checkLexeme(")");

#ifdef DEBUG
  cout << "::: exit print()" << endl;
#endif
}

void Parser::program()
{
#ifdef DEBUG
  cout << "::: entering program()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  m_semanticChecker.enterToScope("global");

  checkLexeme("paquete");

  checkLexeme("principal");
 
  ignoreNewLines();
  unsigned int iterations = 0;
  do
  {
    if (m_currentToken.getLexeme().compare("importar") == 0)
    {
      import();
    }
    else if (m_currentToken.getLexeme().compare("var") == 0)
    {
      variablesDeclaration();
    }
    else if (m_currentToken.getLexeme().compare("const") == 0)
    {
      constantsDeclaration();
    }
    else if (m_currentToken.getLexeme().compare("funcion") == 0)
    {
      functionDeclaration();
    }

    ignoreNewLines();

    if (m_currentToken.getLexeme().compare(",") == 0 ||
        m_currentToken.getToken() == TOKEN_NEWLINE)
    {
      advanceToken();
    }

#ifdef DEBUG
    cout << "::: current lexeme (line " << __LINE__ << "): " <<
        m_currentToken.getLexeme() << endl;
#endif

    ++iterations;
  } while ((m_currentToken.getLexeme().compare("var") == 0       ||
            m_currentToken.getLexeme().compare("importar")  == 0 ||
            m_currentToken.getLexeme().compare("const")  == 0    ||
            m_currentToken.getLexeme().compare("funcion")  == 0) &&
            iterations < m_maxRuleIterations);

  m_semanticChecker.exitCurrentScope();
#ifdef DEBUG
  cout << "::: exit program()" << endl;
#endif
}

void Parser::read()
{
#ifdef DEBUG
  cout << "::: entering read()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  checkLexeme("con");

  m_semanticChecker.checkImported("\"con\"");

  checkLexeme(".");
  checkLexeme("Lee");
  checkLexeme("(");
  advanceToken();
  argumentsList();
  checkLexeme(")");

#ifdef DEBUG
  cout << "::: exit read()" << endl;
#endif
}

void Parser::relationalOperation()
{
#ifdef DEBUG
  cout << "::: entering relationalOperation()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  sumOperation();
  if (m_currentToken.getToken() == TOKEN_RELOP)
  {
    m_semanticChecker.pushOperator(OPERATOR_REL);

    advanceToken();
    sumOperation();
  }

#ifdef DEBUG
  cout << "::: exit relationalOperation()" << endl;
#endif
}

void Parser::returnExpression()
{
#ifdef DEBUG
  cout << "::: entering returnExpression()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  checkLexeme("regresa");

  advanceToken();
  if (m_currentToken.getLexeme().compare(";") != 0 &&
      m_currentToken.getToken() != TOKEN_NEWLINE)
  {
    m_semanticChecker.setReturnCalled(true);

    expression();
  }

#ifdef DEBUG
  cout << "::: exit returnExpression()" << endl;
#endif
}

void Parser::returnType()
{
#ifdef DEBUG
  cout << "::: entering returnType()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;
  
  advanceToken();
  if (m_currentToken.getLexeme().compare("{") != 0)
  {
    m_semanticChecker.setReturnRequired(true);

    if (m_currentToken.getLexeme().compare("(") == 0)
    {
      checkLexeme("(");
      advanceToken();
      if (m_currentToken.getToken() == TOKEN_IDEN)
      {
        unsigned int iterations = 0;
        do
        {
          checkToken(TOKEN_IDEN);
          variablesList();
          checkNativeDataType();
          advanceToken();

          ++iterations;
        } while (m_currentToken.getLexeme().compare(",") == 0 &&
                 iterations < m_maxRuleIterations);
        checkLexeme(")");
      }
      else
      {
        checkNativeDataType();
        checkLexeme(")");
      }
    }
    else 
    {
      m_scanner->moveBackwards();
      checkNativeDataType();

      m_function.setType(
          SymbolData::getStringType(m_scanner->getLastToken().getLexeme()));
    }
  }
  else
  {
    m_semanticChecker.setReturnRequired(false);

    m_scanner->moveBackwards();
  }

#ifdef DEBUG
  cout << "::: exit returnType()" << endl;
#endif
}

void Parser::sign()
{
#ifdef DEBUG
  cout << "::: entering sign()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  if (m_currentToken.getLexeme().compare("-") == 0)
  {
    m_scanner->moveBackwards();
    checkLexeme("-");

    m_semanticChecker.pushOperator(OPERATOR_MINUS);
  }
  term();

#ifdef DEBUG
  cout << "::: exit sign()" << endl;
#endif
}

void Parser::statements()
{
#ifdef DEBUG
  cout << "::: entering statements()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  if (m_currentToken.getLexeme().compare("}") != 0)
  {
    ignoreNewLines();
    unsigned int iterations = 0;
    do
    {
      command();
      ignoreNewLines();
      if (m_currentToken.getLexeme().compare(";") == 0 ||
          m_currentToken.getToken() == TOKEN_NEWLINE)
      {
        advanceToken();
      }
#ifdef DEBUG
      cout << "::: current lexeme (line " << __LINE__ << "): " <<
          m_currentToken.getLexeme() << endl;
#endif

      ++iterations;
    } while ((m_currentToken.getToken() == TOKEN_IDEN               ||
              m_currentToken.getLexeme().compare("var") == 0        ||
              m_currentToken.getLexeme().compare("si") == 0         ||
              m_currentToken.getLexeme().compare("desde") == 0      ||
              m_currentToken.getLexeme().compare("caso") == 0       ||
              m_currentToken.getLexeme().compare("regresa") == 0    ||
              m_currentToken.getLexeme().compare("interrumpe") == 0 ||
              m_currentToken.getLexeme().compare("continua") == 0)  &&
              iterations < m_maxRuleIterations);
  }
#ifdef DEBUG
  cout << "::: exit statements()" << endl;
#endif
}

void Parser::sumOperation()
{
#ifdef DEBUG
  cout << "::: entering sumOperation()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  bool isOperatorFound = false;
  unsigned int iterations = 0;
  do
  {
    isOperatorFound = false;
    multiplication();
    if (m_currentToken.getLexeme().compare("+") == 0 ||
        m_currentToken.getLexeme().compare("-") == 0)
    {
      isOperatorFound = true;

      if (m_currentToken.getLexeme().compare("+") == 0)
      {
        m_semanticChecker.pushOperator(OPERATOR_PLUS);
      }
      else
      {
        m_semanticChecker.pushOperator(OPERATOR_MINUS);
      }

      advanceToken();
    }

    ++iterations;
  } while (isOperatorFound && iterations < m_maxRuleIterations); 

#ifdef DEBUG
  cout << "::: exit sumOperation()" << endl;
#endif
}

void Parser::term()
{
#ifdef DEBUG
  cout << "::: entering term()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

#ifdef DEBUG
  cout << "checking term with lexeme: " << m_currentToken.getLexeme() << endl;
#endif
  if (m_currentToken.getLexeme().compare("(") == 0)
  {
    checkLexeme("(");
    expression();
    checkLexeme(")");
  }
  else if (m_currentToken.getToken() == TOKEN_IDEN)
  {
    advanceToken();
    if (m_currentToken.getLexeme().compare("[") == 0)
    {
      m_scanner->moveBackwards();

      dimension();
      advanceToken();
    }
    else if (m_currentToken.getLexeme().compare("(") == 0)
    {
      m_scanner->moveBackwards();
      functionCall();
    }
    else
    {
      m_scanner->moveBackwards();
      m_semanticChecker.pushVariableType(m_scanner->getLastToken().getLexeme());
      m_scanner->moveForward();
    }
  }
  else if (isLiteral(m_currentToken.getToken()))
  {
#ifdef DEBUG
    cout << "::: pushtype literal: " << m_currentToken.getLexeme() << endl;
#endif
    m_semanticChecker.pushOperand(SymbolData::getTypeChar(
        SymbolData::getLiteralType(m_currentToken.getToken())));
    advanceToken();
  }
  else
  {
    m_errorReporter->writeSyntaxError("identificador o constante");
  }
#ifdef DEBUG
  cout << "::: exit term()" << endl;
#endif
}

void Parser::variablesDeclaration()
{
#ifdef DEBUG
  cout << "::: entering variablesDeclaration()" << endl;
#endif
  if (m_errorReporter->getErrors() >= m_maxErrors)
    return;

  checkLexeme("var");

  advanceToken();
  if (m_currentToken.getLexeme().compare("(") == 0)
  {
    unsigned int iterations = 0;
    do
    {
      variablesList();

      advanceToken();
#ifdef DEBUG
    cout << "::: current lexeme (line " << __LINE__ << "): " <<
        m_currentToken.getLexeme() << endl;
#endif

      ++iterations;
    } while (m_currentToken.getToken() == TOKEN_NEWLINE &&
             iterations < m_maxRuleIterations);
    m_scanner->moveBackwards();
    checkLexeme(")");
  }
  else
  {
    m_scanner->moveBackwards();
    variablesList();
  }
  advanceToken();
#ifdef DEBUG
  cout << "::: exit variablesDeclaration()" << endl;
#endif
}

void Parser::variablesList()
{
#ifdef DEBUG
  cout << "::: entering variablesList()" << endl;
#endif
  string nameWithScope;
  unsigned int iterations = 0;
  do
  {

    checkToken(TOKEN_IDEN);

    SymbolData variable;
    variable.setName(m_scanner->getLastToken().getLexeme());
    variable.setLine(m_scanner->getLastToken().getLine());
    m_variables.push(variable);

    advanceToken();
    if (m_currentToken.getLexeme().compare("[") == 0)
    {
      m_scanner->moveBackwards();
      dimension();
      advanceToken();
    }

#ifdef DEBUG
    cout << "::: current lexeme (line " << __LINE__ << "): " <<
        m_currentToken.getLexeme() << endl;
#endif
    if (m_currentToken.getLexeme().compare(",") != 0)
    {
      m_scanner->moveBackwards();
    }
    ++iterations;
  } while (m_currentToken.getLexeme().compare(",") == 0 &&
           iterations < m_maxRuleIterations);

  checkNativeDataType();

  NativeType_t parameterType =
      SymbolData::getStringType(m_scanner->getLastToken().getLexeme());
  resolveVariablesType(parameterType);

#ifdef DEBUG
  cout << "::: exit variablesList()" << endl;
#endif
}

void Parser::checkToken(TokenType_t expectedToken)
{
  if (m_errorReporter->getErrors() < m_maxErrors)
  {
    advanceToken();
#ifdef DEBUG
      cout << "::: checking for " <<
          TokenLexeme::getTokenString(expectedToken) << ", got: " <<
          TokenLexeme::getTokenString(m_currentToken.getToken()) << ", " <<
          m_currentToken.getLexeme() << endl;
#endif
    if (m_currentToken.getToken() != expectedToken)
    {
      m_errorReporter->writeSyntaxError(expectedToken);
    }
  }
}

void Parser::checkLexeme(const string& expectedLexeme)
{
  if (m_errorReporter->getErrors() < m_maxErrors)
  {
    advanceToken();
#ifdef DEBUG
      cout << "::: checking for " << expectedLexeme << ", got: " <<
          m_currentToken.getLexeme() << endl;
#endif
    if (m_currentToken.getLexeme().compare(expectedLexeme) != 0)
    {
      m_errorReporter->writeSyntaxError(expectedLexeme);
    }
  }
}


void Parser::checkNativeDataType()
{
  if (m_errorReporter->getErrors() < m_maxErrors)
  {
    advanceToken();
#ifdef DEBUG
      cout << "::: checking for native data type, got: " <<
          m_currentToken.getLexeme() << endl;
#endif
    if (!isNativeDataType(m_currentToken.getLexeme()))
    {
      string message = "se esperaba token: \"tipo_dato\", se recibio \"";
      message += TokenLexeme::getTokenString(m_currentToken.getToken());
      message += "\"";

      m_errorReporter->writeErrorWithPosition(message);
    }
  }
}

void Parser::checkLiteral()
{
  if (m_errorReporter->getErrors() < m_maxErrors)
  {
    advanceToken();
#ifdef DEBUG
    cout << "::: checking for constant literal, got: " <<
        m_currentToken.getLexeme() << endl;
#endif
    if (!isLiteral(m_currentToken.getToken()))
    {
      m_errorReporter->writeSyntaxError("constante");
    }
  }
}

void Parser::ignoreNewLines()
{
#ifdef DEBUG
  cout << "::: ignoring newlines()" << endl;
#endif

  unsigned int iterations = 0;
  do
  {
    advanceToken();

    ++iterations;
  } while ((m_currentToken.getToken() == TOKEN_NEWLINE ||
            m_currentToken.getLexeme().compare(";") == 0) &&
            iterations < m_maxRuleIterations);

  m_scanner->moveBackwards();

#ifdef DEBUG
  cout << "::: stopped ignoring newlines()" << endl;
#endif
}

void Parser::advanceToken()
{
  m_currentToken = m_scanner->getCurrentTokenAndAdvance();
  ++m_nTokensProcessed;
}

bool Parser::isNativeDataType(const string& lexeme)
{
    if (lexeme.compare(TYPESTRING_CHAR) == 0    ||
        lexeme.compare(TYPESTRING_INTEGER) == 0 ||
        lexeme.compare(TYPESTRING_FLOAT) == 0   ||
        lexeme.compare(TYPESTRING_BOOL) == 0    ||
        lexeme.compare(TYPESTRING_STRING) == 0)
    {
      return true;
    }
    return false;
}

bool Parser::isLiteral(TokenType_t token)
{
  if (token == TOKEN_DEC       ||
      token == TOKEN_HEX       ||
      token == TOKEN_OCT       ||
      token == TOKEN_FLOAT     ||
      token == TOKEN_CHARCONST ||
      token == TOKEN_STRING)
  {
    return true;
  }

  return false;
}

void Parser::resolveVariablesType(NativeType_t type)
{
  while (!m_variables.empty())
  {
    m_variables.front().setType(type);
#ifdef DEBUG
    cout << "::: size of m_variables: " << m_variables.size() << endl; 
    cout << "::: declaring variable: " << m_variables.front().getName() <<
      " in line: " << m_variables.front().getLine() << endl;
#endif
    m_semanticChecker.declare(m_variables.front());
    m_variables.pop();
  }
}

