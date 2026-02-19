#[
  这是一个Vix语言的格式化工具

  @Author: Fexcode
  @Date: 2026.2.19
]#

import os
import strutils
import sequtils

# 定义 Token 类型
type
  TokenType = enum
    tkKeyword, tkIdentifier, tkNumber, tkString, tkSymbol, tkPunctuation, tkComment, tkNewline, tkEOF
  Token = object
    case kind: TokenType
    of tkKeyword, tkIdentifier, tkSymbol, tkComment, tkString, tkNumber:
      val: string
    of tkPunctuation:
      punct: char
    else: discard

# Vix 关键字列表
const Keywords = [
  "fn", "return", "if", "elif", "else", "while", "for", "in", 
  "struct", "obj", "field", "impl", "extern", "global", "mut", 
  "maybe", "error", "catch", "as", "and", "or", "public", "pub"
]

# 操作符符号 (注意顺序：.. 必须在 . 前面，以便优先匹配)
const Symbols = [
  "..", "->", "==", "!=", "<=", ">=", "&&", "||", "+=", "-=", "*=", "/=",
  "!", "+", "-", "*", "/", "%", "=", "<", ">", "&", "@", "."
]

# --- 词法分析器 ---

proc isKeyword(s: string): bool =
  return s in Keywords

proc tokenize(code: string): seq[Token] =
  var i = 0
  let n = code.len
  
  proc peek(off: int = 0): char =
    let idx = i + off
    if idx < n: result = code[idx] else: result = '\0'

  proc advance(): char =
    if i < n:
      result = code[i]
      inc i
    else:
      result = '\0'

  while i < n:
    let c = peek()
    
    if c in [' ', '\t', '\r']:
      discard advance()
      continue
    
    if c == '\n':
      result.add Token(kind: tkNewline)
      discard advance()
      continue

    # 注释处理
    if c == '/' and peek(1) == '/':
      var comment = ""
      while i < n and peek() != '\n':
        comment.add(advance())
      result.add Token(kind: tkComment, val: comment)
      continue
    
    if c == '/' and peek(1) == '*':
      var comment = "/*"
      inc i, 2
      while i < n:
        if peek() == '*' and peek(1) == '/':
          comment.add "*/"
          inc i, 2
          break
        comment.add(advance())
      result.add Token(kind: tkComment, val: comment)
      continue

    # 字符串处理
    if c == '"':
      var s = "\""
      discard advance()
      while i < n:
        let ch = advance()
        s.add(ch)
        if ch == '"': break
        if ch == '\\':
          if i < n: s.add(advance())
      result.add Token(kind: tkString, val: s)
      continue

    # 数字处理
    if c in Digits:
      var s = ""
      while i < n and (peek() in Digits or peek() == '.'):
        s.add(advance())
      result.add Token(kind: tkNumber, val: s)
      continue

    # 标识符与关键字
    if c in Letters or c == '_':
      var s = ""
      while i < n and (peek() in Letters + Digits or peek() == '_'):
        s.add(advance())
      if isKeyword(s):
        result.add Token(kind: tkKeyword, val: s)
      else:
        result.add Token(kind: tkIdentifier, val: s)
      continue

    # 符号处理 (最长匹配原则)
    var matchedSymbol = ""
    for sym in Symbols:
      if code.substr(i, i+sym.len-1) == sym:
        if sym.len > matchedSymbol.len:
          matchedSymbol = sym
    
    if matchedSymbol.len > 0:
      result.add Token(kind: tkSymbol, val: matchedSymbol)
      inc i, matchedSymbol.len
      continue

    # 标点符号
    if c in {'{', '}', '(', ')', '[', ']', ',', ';', ':'}:
      result.add Token(kind: tkPunctuation, punct: c)
      discard advance()
      continue

    discard advance()

  result.add Token(kind: tkEOF)

# --- 格式化器 ---

proc formatCode(tokens: seq[Token]): string =
  var indentLevel = 0
  var currentLine = ""
  var outputBuffer = ""

  proc addIndent() =
    currentLine.add(repeat("    ", indentLevel))

  proc flushLine() =
    outputBuffer.add(currentLine.strip(leading=false, trailing=true))
    outputBuffer.add("\n")
    currentLine = ""

  var prevKind = tkPunctuation 
  var justFlushed = true

  for i, token in tokens:
    case token.kind
    of tkNewline:
      # 括号不换行：如果下一个是 '{'，忽略当前换行
      if i + 1 < tokens.len and tokens[i+1].kind == tkPunctuation and tokens[i+1].punct == '{':
        discard 
      else:
        if currentLine.len > 0:
          flushLine()
          justFlushed = true
        prevKind = tkNewline

    of tkEOF:
      if currentLine.len > 0: flushLine()

    of tkComment:
      if justFlushed: addIndent()
      if not justFlushed: currentLine.add(" ")
      currentLine.add(token.val)
      if token.val.startsWith("//"):
        flushLine()
        justFlushed = true
      prevKind = tkComment

    of tkKeyword:
      if justFlushed: addIndent()
      else: 
        if prevKind notin {tkSymbol, tkPunctuation, tkNewline}:
          currentLine.add(" ")
      currentLine.add(token.val)
      currentLine.add(" ")
      justFlushed = false
      prevKind = tkKeyword

    of tkIdentifier:
      if justFlushed: addIndent()
      elif prevKind in {tkKeyword, tkIdentifier, tkNumber, tkSymbol}:
        # 特殊处理：如果前一个是 . & @ !，不加空格
        if prevKind == tkSymbol:
           let prevSym = tokens[i-1].val
           if prevSym in [".", "&", "@", "!"]:
              discard
           else:
              currentLine.add(" ")
        elif prevKind == tkKeyword and tokens[i-1].val == "fn":
           discard
        elif prevKind != tkPunctuation:
           currentLine.add(" ")
        elif tokens[i-1].punct in {'(', '[', '{'}:
           discard
        else:
           currentLine.add(" ")
      
      currentLine.add(token.val)
      justFlushed = false
      prevKind = tkIdentifier

    of tkNumber:
      if justFlushed: addIndent()
      elif prevKind notin {tkPunctuation, tkSymbol}:
        currentLine.add(" ")
      currentLine.add(token.val)
      justFlushed = false
      prevKind = tkNumber

    of tkString:
      if justFlushed: addIndent()
      elif prevKind notin {tkPunctuation, tkSymbol, tkKeyword}: 
        if prevKind == tkIdentifier:
           currentLine.add(" ")
      currentLine.add(token.val)
      justFlushed = false
      prevKind = tkString

    of tkSymbol:
      if justFlushed: addIndent()
      
      # 处理点号：无空格
      if token.val == ".":
         currentLine.add(".")
      
      # 处理感叹号 (支持 replace!() 语法)
      elif token.val == "!":
        currentLine.add("!")
        # 如果后面紧跟 '('，不加空格；否则加空格 (如 ! true 这种少见写法)
        if i + 1 < tokens.len and tokens[i+1].kind == tkPunctuation and tokens[i+1].punct == '(':
           discard
        else:
           currentLine.add(" ")

      # 处理前缀操作符 & @
      elif token.val in ["&", "@"]:
         currentLine.add(token.val)

      # 处理范围操作符 ..
      elif token.val == "..":
         if prevKind notin {tkNewline} and not justFlushed:
            currentLine.add(" ")
         currentLine.add("..")
         currentLine.add(" ")

      # 其他符号
      else:
        if prevKind != tkNewline and not justFlushed:
          currentLine.add(" ")
        currentLine.add(token.val)
        currentLine.add(" ")
      
      justFlushed = false
      prevKind = tkSymbol

    of tkPunctuation:
      if token.punct == '{':
        if justFlushed:
           addIndent()
        else:
           currentLine.add(" ") 
        
        currentLine.add("{")
        flushLine()
        inc indentLevel
        justFlushed = true
        prevKind = tkPunctuation

      elif token.punct == '}':
        if currentLine.strip().len > 0: flushLine()
        dec indentLevel
        addIndent()
        currentLine.add("}")
        flushLine()
        justFlushed = true
        prevKind = tkPunctuation

      elif token.punct == ',':
        currentLine.add(", ")
        prevKind = tkPunctuation
      
      elif token.punct == ';':
        # 【修复】分号不换行，改为加空格，支持 for 循环语法
        currentLine.add("; ")
        prevKind = tkPunctuation
      
      elif token.punct == ':':
        currentLine.add(": ")
        prevKind = tkPunctuation

      else: # (, ), [
        if justFlushed: 
           addIndent()
        
        if token.punct in {'(', '['}:
           if prevKind == tkIdentifier:
             discard
           elif prevKind in {tkKeyword, tkNumber, tkString}:
             if prevKind != tkKeyword:
               currentLine.add(" ")
           currentLine.add(token.punct)
        else: # ), ]
           currentLine.add(token.punct)
        
        justFlushed = false
        prevKind = tkPunctuation

  result = outputBuffer.strip()

# --- 文件处理辅助函数 ---

proc processFile(filename: string, outputToConsole: bool) =
  try:
    let content = readFile(filename)
    let tokens = tokenize(content)
    let formatted = formatCode(tokens)
    
    if outputToConsole:
      echo formatted
    else:
      # 原地修改
      writeFile(filename, formatted & "\n")
      echo "格式化完成: ", filename
  except IOError:
    echo "错误: 无法读写文件 ", filename

# --- 主程序 ---

proc main() =
  var outputToConsole = false
  var targetPath = ""

  # 解析参数
  for i in 1..paramCount():
    let arg = paramStr(i)
    if arg == "-o" or arg == "--output":
      outputToConsole = true
    elif targetPath.len == 0:
      targetPath = arg
    else:
      echo "用法: vform [-o|--output] <文件名或目录>"
      quit(1)

  if targetPath.len == 0:
    echo "用法: vform [-o|--output] <文件名或目录>"
    echo "说明:"
    echo "  默认行为: 原地修改文件"
    echo "  -o/--output: 输出到控制台而不修改文件"
    echo "  支持传入目录，将递归格式化所有 .vix 文件"
    quit(1)

  if dirExists(targetPath):
    # 处理目录
    echo "正在扫描目录: ", targetPath
    var count = 0
    for file in walkDirRec(targetPath):
      if file.endsWith(".vix"):
        processFile(file, outputToConsole)
        inc count
    if count == 0:
      echo "未找到 .vix 文件。"
    else:
      echo "总计处理 ", count, " 个文件。"
  elif fileExists(targetPath):
    # 处理单个文件
    processFile(targetPath, outputToConsole)
  else:
    echo "错误: 路径不存在 ", targetPath

when isMainModule:
  main()
