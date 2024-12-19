grammar HA;

@header {
    #include "AST/AST.h"
}

@parser::members {
    AST::SourceInterval toSourceInterval(antlr4::Token *start, antlr4::Token *stop) {
        return AST::SourceInterval{
            {start->getLine(), start->getCharPositionInLine()},
            {stop->getLine(), stop->getCharPositionInLine() + stop->getText().size()}
        };
    }

    AST::SourceInterval toSourceInterval(antlr4::Token *tok) {
        return toSourceInterval(tok, tok);
    }
}

file : (funDef | varDef)+;

localVarDecl returns [std::unique_ptr<AST::LocalVarDeclNode> Result]
    : Start=IDENT { std::string Name = $Start->getText(); AST::PType VarType; } (':' type { VarType = std::move($type.Result); })?
      { $Result = std::make_unique<AST::LocalVarDeclNode>(toSourceInterval($Start), std::move(Name), std::move(VarType)); }
    ;

funDef returns [std::unique_ptr<AST::FunctionDefNode> Result]
    : Start='fun' IDENT '(' { std::string Name = $IDENT->getText(); std::vector<std::unique_ptr<AST::LocalVarDeclNode>> Params; }
        (
            localVarDecl { Params.push_back(std::move($localVarDecl.Result)); }
            (
                ',' localVarDecl { Params.push_back(std::move($localVarDecl.Result)); }
            )* ','?
        )?
    ')' { AST::PType RetType; } (':' type { RetType = std::move($type.Result); })? '=' expr
    { $Result = std::make_unique<AST::FunctionDefNode>(toSourceInterval($Start, $expr.stop), std::move(Name), std::move(Params), std::move(RetType), std::move($expr.Result)); }
    ;

varDef returns [std::unique_ptr<AST::LocalVarDefNode> Result]
    : localVarDef { $Result = std::move($localVarDef.Result); }
    ;

expr returns [AST::PExprNode Result]
    : First=atomExpr { std::vector<AST::PExprNode> Exprs; Exprs.push_back(std::move($atomExpr.Result)); }
        (
            ';' Last=atomExpr { Exprs.push_back(std::move($atomExpr.Result)); }
        )* (LastT=';')?
    {
        $Result = std::make_unique<AST::BlockExprNode>(
            toSourceInterval($First.start, $LastT ? $LastT : $Last.stop ? $Last.stop : $First.stop),
            std::move(Exprs)
        );
    }
    ;

atomExpr returns [AST::PExprNode Result]
    : letInExpr { $Result = std::move($letInExpr.Result); }
    | ifExpr { $Result = std::move($ifExpr.Result); }
    | allocaExpr { $Result = std::move($allocaExpr.Result); }
    | Callee=atomExpr { std::vector<AST::PExprNode> Args; } '('
        (
            atomExpr { Args.push_back(std::move($atomExpr.Result)); }
            (',' atomExpr { Args.push_back(std::move($atomExpr.Result)); })*
            ','?
        )?
    LastT=')' { $Result = std::make_unique<AST::CallExprNode>(toSourceInterval($Callee.start, $LastT), std::move($Callee.Result), std::move(Args)); }
    | First='(' Last=')' { $Result = std::make_unique<AST::ProductExprNode>(toSourceInterval($First, $Last), std::vector<AST::PExprNode>{}); }
    | '(' expr ')' { $Result = std::move($expr.Result); }
    | First='(' atomExpr ',' Last=')'
    {
        std::vector<AST::PExprNode> Elements;
        Elements.push_back(std::move($atomExpr.Result));
        $Result = std::make_unique<AST::ProductExprNode>(toSourceInterval($First, $Last), std::move(Elements));
    }
    | First='(' atomExpr { std::vector<AST::PExprNode> Elements; Elements.push_back(std::move($atomExpr.Result)); } (
            ',' atomExpr { Elements.push_back(std::move($atomExpr.Result)); }
        )+ ','? Last=')' { $Result = std::make_unique<AST::ProductExprNode>(toSourceInterval($First, $Last), std::move(Elements)); }
    | IDENT { $Result = std::make_unique<AST::IdentExprNode>(toSourceInterval($IDENT), $IDENT->getText()); }
    | literal { $Result = std::move($literal.Result); }
    ;

localVarDef returns [std::unique_ptr<AST::LocalVarDefNode> Result]
    : localVarDecl '=' atomExpr
    {
        $Result = std::make_unique<AST::LocalVarDefNode>(
            toSourceInterval($localVarDecl.start, $atomExpr.stop),
            std::move($localVarDecl.Result->Name),
            std::move($localVarDecl.Result->RetType),
            std::move($atomExpr.Result)
        );
    }
    ;

letInExpr returns [AST::PLetInExprNode Result]
    : Start='let' localVarDef { std::vector<AST::PLocalVarDefNode> LocalVars; LocalVars.push_back(std::move($localVarDef.Result)); }
        (
            ',' localVarDef { LocalVars.push_back(std::move($localVarDef.Result)); }
        )* ','? 'in' expr
    { $Result = std::make_unique<AST::LetInExprNode>(toSourceInterval($Start, $expr.stop), std::move(LocalVars), std::move($expr.Result)); }
    ;

ifExpr returns [AST::PIfExprNode Result]
    : Start='if' Cond=atomExpr 'then' Then=expr
    { AST::PExprNode ElseExpr = nullptr; }
        (
            'else' expr { ElseExpr = std::move($expr.Result); }
        )? LastT='fi'
    {
        if (!ElseExpr) {
            ElseExpr = std::make_unique<AST::ProductExprNode>(toSourceInterval($LastT), std::vector<AST::PExprNode>{});
        }
        $Result = std::make_unique<AST::IfExprNode>(toSourceInterval($Start, $LastT), std::move($Cond.Result), std::move($Then.Result), std::move(ElseExpr));
    }
    ;

allocaExpr returns [AST::PAllocaExprNode Result]
    : Start='alloca' type '[' expr Last=']' { $Result = std::make_unique<AST::AllocaExprNode>(toSourceInterval($Start, $Last), std::move($type.Result), std::move($expr.Result)); }
    ;

literal returns [AST::PIntLiteralExprNode Result]
    : INT { $Result = std::make_unique<AST::IntLiteralExprNode>(toSourceInterval($INT), atoi($INT->getText().c_str())); }
    | NEG_INT { $Result = std::make_unique<AST::IntLiteralExprNode>(toSourceInterval($NEG_INT), atoi($NEG_INT->getText().c_str())); }
    ;

type returns [AST::PType Result]
    : 'int' { $Result = std::make_shared<AST::BuiltinAtomType>(AST::BuiltinAtomTypeKind::INT); }
    | '[' type ']' { $Result = std::make_shared<AST::ArrayType>(std::move($type.Result)); }
    | '(' { std::vector<AST::PType> Elements; }
        (
            type { Elements.push_back(std::move($type.Result)); }
            (',' type { Elements.push_back(std::move($type.Result)); })* ','?
        )?
    ')' { $Result = std::make_shared<AST::ProductType>(std::move(Elements)); }
    | Params=type '->' Ret=type {
        if (auto PP = std::dynamic_pointer_cast<AST::ProductType>($Params.Result)) {
            $Result = std::make_shared<AST::FunctionType>(std::move($Ret.Result), AST::PProductType(PP));
        } else {
            std::vector<AST::PType> ParamsTypes;
            ParamsTypes.push_back(std::move($Params.Result));
            $Result = std::make_shared<AST::FunctionType>(std::move($Ret.Result), std::make_shared<AST::ProductType>(std::move(ParamsTypes)));
        }
    }
    ;

fragment IDENT_START: [a-zA-Z!%^&*\-=_+<>/];
fragment DIGIT: [0-9];
INT: DIGIT+;
NEG_INT: '-' DIGIT+;
IDENT: IDENT_START (IDENT_START | DIGIT)*;
WS: [ \t\r\n]+ -> skip;

COMMENT
    : '#-' .*? '-#' -> skip
;

LINE_COMMENT
    : '#' ~[\r\n]* -> skip
;

