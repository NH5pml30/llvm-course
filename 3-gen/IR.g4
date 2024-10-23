grammar IR;

file :
    (
        sourceFilename |
        targetDatalayout |
        targetTriple |
        funcDef |
        funcDecl |
        attrDef |
        metaDef
    )*;

sourceFilename :
    'source_filename' '=' STRING
    ;

targetDatalayout :
    'target datalayout' '=' STRING;

targetTriple :
    'target triple' '=' STRING;

funcDef :
    'define' globalSpec? type GLOBAL '('
    (
        params+=paramDef
        (
            ','
            params+=paramDef
        )*
    )?
    ')'
    globalAttr* attrRef?
    '{'
    (
        basicBlocks+=basicBlock
    )*
    '}';

globalSpec :
    'dso_local' |
    'dso_preemptable';

globalAttr :
    'local_unnamed_addr';

funcDecl :
    'declare' globalSpec? type GLOBAL '('
    (
        params+=paramDecl
        (
            ','
            params+=paramDecl
        )*
    )?
    ')'
    globalAttr* attrRef?
    ;

regName :
    IDENT
    |
    DECIMAL;

basicBlock :
    (
        regName ':'
    )?
    (
        innerInsts+=innerInst
    )*
    termInst;

value :
    REG |
    const;

const :
    DECIMAL |
    NEG_DECIMAL |
    fls='false' |
    tru='true';

termInst :
    ret='ret' rt=type rv=value?
    |
    'br' bl='label' br=REG metaRef?
    |
    'br' bt=type bv=value ',' 'label' brt=REG ',' 'label' brf=REG metaRef?;

innerInst :
    (
        lhs=REG '='
    )?
    (
        phi='phi' phiType=type
        '[' values+=value ',' labels+=REG ']'
        (
            ','
            '[' values+=value ',' labels+=REG ']'
        )+
        |
        alloca='alloca' allocaType=type ',' 'align' allocaAlign=DECIMAL
        |
        'tail'? call='call' type GLOBAL '('
        (
            argTypes+=type paramAttr* argValues+=value
            (
                ','
                argTypes+=type paramAttr* argValues+=value
            )*
        )?
        ')'
        attrRef?
        |
        binaryOp
        (
            binaryOpsAttrs+=binaryOpsAttr
        )*
        bt=type blhs=value ',' brhs=value
        |
        store='store' storeType=type storeValue=value ',' type storeReg=REG ',' 'align' storeAlign=DECIMAL metaRef? |
        load='load' loadType=type ',' type loadReg=REG ',' 'align' loadAlign=DECIMAL metaRef? |
        gep='getelementptr' inbounds='inbounds'? gepType=type ',' type gepReg=REG (',' gepTypes+=type gepIdxs+=value)* |
        icmp='icmp' icmpCond icmpType=type icmpLhs=value ',' icmpRhs=value |
        select='select' selectType=type selectValue=value ',' selLhsType=type selLhs=value ',' selRhsType=type selRhs=value
    )
    ;

binaryOp :
    srem='srem' |
    add='add'  |
    sdiv='sdiv' |
    shl='shl';

binaryOpsAttr :
    nuw='nuw' |
    nsw='nsw';

icmpCond :
    ult='ult' |
    slt='slt' |
    sgt='sgt' |
    eq='eq';

leafType :
    i64='i64' |
    i32='i32' |
    i8='i8'   |
    i1='i1'   |
    p='ptr'   |
    v='void'  ;

type :
    leafType |
    '[' d=DECIMAL 'x' type ']';

paramAttr :
    'nocapture' |
    'noundef' |
    'writeonly' |
    'nonnull' |
    'align' DECIMAL |
    'dereferenceable' '(' DECIMAL ')' |
    'immarg';

paramDef :
    type paramAttr* REG;

paramDecl :
    type paramAttr*;

metaRef :
    ',' META META;

metaDef :
    META '=' 'distinct'? '!' '{' ~'}'* '}';

attrRef :
    '#' DECIMAL;

attrDef :
    'attributes' '#' DECIMAL '=' '{' ~'}'* '}';

// lexer

WS : [ \t\r\n\u000C]+ -> skip;
LINE_COMMENT : ';' ~[\r\n]* -> skip;

fragment LETTER : [A-Z] | [a-z] | '_' | '.';
fragment DIGIT : [0-9];
fragment STRING_CHAR : ~'"';

DECIMAL : DIGIT+;
NEG_DECIMAL : '-' DIGIT+;
REG: '%' (LETTER | DIGIT)+;
GLOBAL: '@' (LETTER | DIGIT)+;
META: '!' (LETTER | DIGIT)+;
STRING : '"' STRING_CHAR* '"';
IDENT : LETTER (LETTER | DIGIT)*;
