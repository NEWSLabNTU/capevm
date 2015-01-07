/* Generated By:JavaCC: Do not edit this line. ProbeParserTokenManager.java */
package avrora.test.probes;

import java.io.IOException;
import java.io.PrintStream;


public class ProbeParserTokenManager implements ProbeParserConstants
{
  public  PrintStream debugStream = System.out;
  public  void setDebugStream(PrintStream ds) { debugStream = ds; }
private int jjStopStringLiteralDfa_0(int pos, long active0)
{
   switch (pos)
   {
      case 0:
         if ((active0 & 0x3fc000L) != 0L)
         {
            jjmatchedKind = 26;
            return 6;
         }
         return -1;
      case 1:
         if ((active0 & 0x3fc000L) != 0L)
         {
            jjmatchedKind = 26;
            jjmatchedPos = 1;
            return 6;
         }
         return -1;
      case 2:
         if ((active0 & 0x3fc000L) != 0L)
         {
            jjmatchedKind = 26;
            jjmatchedPos = 2;
            return 6;
         }
         return -1;
      case 3:
         if ((active0 & 0x20000L) != 0L)
            return 6;
         if ((active0 & 0x3dc000L) != 0L)
         {
            jjmatchedKind = 26;
            jjmatchedPos = 3;
            return 6;
         }
         return -1;
      case 4:
         if ((active0 & 0x1c000L) != 0L)
            return 6;
         if ((active0 & 0x3c0000L) != 0L)
         {
            jjmatchedKind = 26;
            jjmatchedPos = 4;
            return 6;
         }
         return -1;
      case 5:
         if ((active0 & 0x100000L) != 0L)
         {
            jjmatchedKind = 26;
            jjmatchedPos = 5;
            return 6;
         }
         if ((active0 & 0x2c0000L) != 0L)
            return 6;
         return -1;
      default :
         return -1;
   }
}
private int jjStartNfa_0(int pos, long active0)
{
   return jjMoveNfa_0(jjStopStringLiteralDfa_0(pos, active0), pos + 1);
}
private int jjStopAtPos(int pos, int kind)
{
   jjmatchedKind = kind;
   jjmatchedPos = pos;
   return pos + 1;
}
private int jjStartNfaWithStates_0(int pos, int kind, int state)
{
   jjmatchedKind = kind;
   jjmatchedPos = pos;
   try { curChar = input_stream.readChar(); }
   catch(IOException e) { return pos + 1; }
   return jjMoveNfa_0(state, pos + 1);
}
private int jjMoveStringLiteralDfa0_0()
{
   switch(curChar)
   {
      case 35:
         return jjStopAtPos(0, 6);
      case 59:
         return jjStopAtPos(0, 24);
      case 97:
         return jjMoveStringLiteralDfa1_0(0x100000L);
      case 101:
         return jjMoveStringLiteralDfa1_0(0x8000L);
      case 105:
         return jjMoveStringLiteralDfa1_0(0x40000L);
      case 109:
         return jjMoveStringLiteralDfa1_0(0x20000L);
      case 112:
         return jjMoveStringLiteralDfa1_0(0x4000L);
      case 114:
         return jjMoveStringLiteralDfa1_0(0x280000L);
      case 119:
         return jjMoveStringLiteralDfa1_0(0x10000L);
      case 123:
         return jjStopAtPos(0, 22);
      case 124:
         return jjStopAtPos(0, 25);
      case 125:
         return jjStopAtPos(0, 23);
      default :
         return jjMoveNfa_0(0, 0);
   }
}
private int jjMoveStringLiteralDfa1_0(long active0)
{
   try { curChar = input_stream.readChar(); }
   catch(IOException e) {
      jjStopStringLiteralDfa_0(0, active0);
      return 1;
   }
   switch(curChar)
   {
      case 97:
         return jjMoveStringLiteralDfa2_0(active0, 0x30000L);
      case 100:
         return jjMoveStringLiteralDfa2_0(active0, 0x100000L);
      case 101:
         return jjMoveStringLiteralDfa2_0(active0, 0x280000L);
      case 110:
         return jjMoveStringLiteralDfa2_0(active0, 0x40000L);
      case 114:
         return jjMoveStringLiteralDfa2_0(active0, 0x4000L);
      case 118:
         return jjMoveStringLiteralDfa2_0(active0, 0x8000L);
      default :
         break;
   }
   return jjStartNfa_0(0, active0);
}
private int jjMoveStringLiteralDfa2_0(long old0, long active0)
{
   if (((active0 &= old0)) == 0L)
      return jjStartNfa_0(0, old0);
   try { curChar = input_stream.readChar(); }
   catch(IOException e) {
      jjStopStringLiteralDfa_0(1, active0);
      return 2;
   }
   switch(curChar)
   {
      case 101:
         return jjMoveStringLiteralDfa3_0(active0, 0x8000L);
      case 105:
         return jjMoveStringLiteralDfa3_0(active0, 0x20000L);
      case 109:
         return jjMoveStringLiteralDfa3_0(active0, 0x80000L);
      case 111:
         return jjMoveStringLiteralDfa3_0(active0, 0x4000L);
      case 115:
         return jjMoveStringLiteralDfa3_0(active0, 0x240000L);
      case 116:
         return jjMoveStringLiteralDfa3_0(active0, 0x10000L);
      case 118:
         return jjMoveStringLiteralDfa3_0(active0, 0x100000L);
      default :
         break;
   }
   return jjStartNfa_0(1, active0);
}
private int jjMoveStringLiteralDfa3_0(long old0, long active0)
{
   if (((active0 &= old0)) == 0L)
      return jjStartNfa_0(1, old0);
   try { curChar = input_stream.readChar(); }
   catch(IOException e) {
      jjStopStringLiteralDfa_0(2, active0);
      return 3;
   }
   switch(curChar)
   {
      case 97:
         return jjMoveStringLiteralDfa4_0(active0, 0x100000L);
      case 98:
         return jjMoveStringLiteralDfa4_0(active0, 0x4000L);
      case 99:
         return jjMoveStringLiteralDfa4_0(active0, 0x10000L);
      case 101:
         return jjMoveStringLiteralDfa4_0(active0, 0x40000L);
      case 110:
         if ((active0 & 0x20000L) != 0L)
            return jjStartNfaWithStates_0(3, 17, 6);
         return jjMoveStringLiteralDfa4_0(active0, 0x8000L);
      case 111:
         return jjMoveStringLiteralDfa4_0(active0, 0x80000L);
      case 117:
         return jjMoveStringLiteralDfa4_0(active0, 0x200000L);
      default :
         break;
   }
   return jjStartNfa_0(2, active0);
}
private int jjMoveStringLiteralDfa4_0(long old0, long active0)
{
   if (((active0 &= old0)) == 0L)
      return jjStartNfa_0(2, old0);
   try { curChar = input_stream.readChar(); }
   catch(IOException e) {
      jjStopStringLiteralDfa_0(3, active0);
      return 4;
   }
   switch(curChar)
   {
      case 101:
         if ((active0 & 0x4000L) != 0L)
            return jjStartNfaWithStates_0(4, 14, 6);
         break;
      case 104:
         if ((active0 & 0x10000L) != 0L)
            return jjStartNfaWithStates_0(4, 16, 6);
         break;
      case 108:
         return jjMoveStringLiteralDfa5_0(active0, 0x200000L);
      case 110:
         return jjMoveStringLiteralDfa5_0(active0, 0x100000L);
      case 114:
         return jjMoveStringLiteralDfa5_0(active0, 0x40000L);
      case 116:
         if ((active0 & 0x8000L) != 0L)
            return jjStartNfaWithStates_0(4, 15, 6);
         break;
      case 118:
         return jjMoveStringLiteralDfa5_0(active0, 0x80000L);
      default :
         break;
   }
   return jjStartNfa_0(3, active0);
}
private int jjMoveStringLiteralDfa5_0(long old0, long active0)
{
   if (((active0 &= old0)) == 0L)
      return jjStartNfa_0(3, old0);
   try { curChar = input_stream.readChar(); }
   catch(IOException e) {
      jjStopStringLiteralDfa_0(4, active0);
      return 5;
   }
   switch(curChar)
   {
      case 99:
         return jjMoveStringLiteralDfa6_0(active0, 0x100000L);
      case 101:
         if ((active0 & 0x80000L) != 0L)
            return jjStartNfaWithStates_0(5, 19, 6);
         break;
      case 116:
         if ((active0 & 0x40000L) != 0L)
            return jjStartNfaWithStates_0(5, 18, 6);
         else if ((active0 & 0x200000L) != 0L)
            return jjStartNfaWithStates_0(5, 21, 6);
         break;
      default :
         break;
   }
   return jjStartNfa_0(4, active0);
}
private int jjMoveStringLiteralDfa6_0(long old0, long active0)
{
   if (((active0 &= old0)) == 0L)
      return jjStartNfa_0(4, old0);
   try { curChar = input_stream.readChar(); }
   catch(IOException e) {
      jjStopStringLiteralDfa_0(5, active0);
      return 6;
   }
   switch(curChar)
   {
      case 101:
         if ((active0 & 0x100000L) != 0L)
            return jjStartNfaWithStates_0(6, 20, 6);
         break;
      default :
         break;
   }
   return jjStartNfa_0(5, active0);
}
private void jjCheckNAdd(int state)
{
   if (jjrounds[state] != jjround)
   {
      jjstateSet[jjnewStateCnt++] = state;
      jjrounds[state] = jjround;
   }
}
private void jjCheckNAddStates(int start, int end)
{
   do {
      jjCheckNAdd(jjnextStates[start]);
   } while (start++ != end);
}
    private int jjMoveNfa_0(int startState, int curPos)
    {
       int startsAt = 0;
       jjnewStateCnt = 12;
       int i = 1;
       jjstateSet[0] = startState;
       int kind = 0x7fffffff;
        while (true) {
            if (++jjround == 0x7fffffff) ReInitRounds();
            if (curChar < 64) {
                long l = 1L << curChar;
                do {
                    switch (jjstateSet[--i]) {
                        case 0:
                            if ((0x3fe000000000000L & l) != 0L) {
                                if (kind > 9) kind = 9;
                                jjCheckNAdd(2);
                            } else if (curChar == 48) {
                                if (kind > 9) kind = 9;
                                jjCheckNAddStates(0, 2);
                            } else if (curChar == 36) jjCheckNAdd(4);
                            else if (curChar == 45) jjstateSet[jjnewStateCnt++] = 1;
                            break;
                        case 1:
                            if ((0x3fe000000000000L & l) == 0L) break;
                            if (kind > 9) kind = 9;
                            jjCheckNAdd(2);
                            break;
                        case 2:
                            if ((0x3ff000000000000L & l) == 0L) break;
                            if (kind > 9) kind = 9;
                            jjCheckNAdd(2);
                            break;
                        case 3:
                            if (curChar == 36) jjCheckNAdd(4);
                            break;
                        case 4:
                            if ((0x3ff000000000000L & l) == 0L) break;
                            if (kind > 9) kind = 9;
                            jjCheckNAdd(4);
                            break;
                        case 6:
                            if ((0x3ff400000000000L & l) == 0L) break;
                            if (kind > 26) kind = 26;
                            jjstateSet[jjnewStateCnt++] = 6;
                            break;
                        case 7:
                            if (curChar != 48) break;
                            if (kind > 9) kind = 9;
                            jjCheckNAddStates(0, 2);
                            break;
                        case 10:
                            if ((0x3000000000000L & l) == 0L) break;
                            if (kind > 9) kind = 9;
                            jjstateSet[jjnewStateCnt++] = 10;
                            break;
                        case 11:
                            if ((0xff000000000000L & l) == 0L) break;
                            if (kind > 9) kind = 9;
                            jjCheckNAdd(11);
                            break;
                        default :
                            break;
                    }
                } while (i != startsAt);
            } else if (curChar < 128) {
                long l = 1L << (curChar & 077);
                do {
                    switch (jjstateSet[--i]) {
                        case 0:
                        case 6:
                            if ((0x7fffffe87fffffeL & l) == 0L) break;
                            if (kind > 26) kind = 26;
                            jjCheckNAdd(6);
                            break;
                        case 4:
                            if ((0x7e0000007eL & l) == 0L) break;
                            if (kind > 9) kind = 9;
                            jjCheckNAdd(4);
                            break;
                        case 8:
                            if ((0x100000001000000L & l) != 0L) jjCheckNAdd(4);
                            break;
                        case 9:
                            if ((0x400000004L & l) != 0L) jjstateSet[jjnewStateCnt++] = 10;
                            break;
                        default :
                            break;
                    }
                } while (i != startsAt);
            } else {
                do {
                    switch (jjstateSet[--i]) {
                        default :
                            break;
                    }
                } while (i != startsAt);
            }
            if (kind != 0x7fffffff) {
                jjmatchedKind = kind;
                jjmatchedPos = curPos;
                kind = 0x7fffffff;
            }
            ++curPos;
            if ((i = jjnewStateCnt) == (startsAt = 12 - (jjnewStateCnt = startsAt))) return curPos;
            try {
                curChar = input_stream.readChar();
            } catch (IOException e) {
                return curPos;
            }
        }
    }
private int jjMoveStringLiteralDfa0_1()
{
   return jjMoveNfa_1(0, 0);
}
private int jjMoveNfa_1(int startState, int curPos)
{
   int startsAt = 0;
   jjnewStateCnt = 3;
   int i = 1;
   jjstateSet[0] = startState;
   int kind = 0x7fffffff;
    while (true) {
        if (++jjround == 0x7fffffff) ReInitRounds();
        if (curChar < 64) {
            long l = 1L << curChar;
            do {
                switch (jjstateSet[--i]) {
                    case 0:
                        if ((0x2400L & l) != 0L) {
                            if (kind > 7) kind = 7;
                        }
                        if (curChar == 13) jjstateSet[jjnewStateCnt++] = 1;
                        break;
                    case 1:
                        if (curChar == 10 && kind > 7) kind = 7;
                        break;
                    case 2:
                        if (curChar == 13) jjstateSet[jjnewStateCnt++] = 1;
                        break;
                    default :
                        break;
                }
            } while (i != startsAt);
        } else {
            do {
                switch (jjstateSet[--i]) {
                    default :
                        break;
                }
            } while (i != startsAt);
        }
        if (kind != 0x7fffffff) {
            jjmatchedKind = kind;
            jjmatchedPos = curPos;
            kind = 0x7fffffff;
        }
        ++curPos;
        if ((i = jjnewStateCnt) == (startsAt = 3 - (jjnewStateCnt = startsAt))) return curPos;
        try {
            curChar = input_stream.readChar();
        } catch (IOException e) {
            return curPos;
        }
    }
}
static final int[] jjnextStates = {
   8, 9, 11,
};
public static final String[] jjstrLiteralImages = {
"", null, null, null, null, null, null, null, null, null, null, null, null,
null, "\160\162\157\142\145", "\145\166\145\156\164", "\167\141\164\143\150",
"\155\141\151\156", "\151\156\163\145\162\164", "\162\145\155\157\166\145",
"\141\144\166\141\156\143\145", "\162\145\163\165\154\164", "\173", "\175", "\73", "\174", null, null, null, };
public static final String[] lexStateNames = {
   "DEFAULT",
   "IN_SINGLE_LINE_COMMENT",
};
public static final int[] jjnewLexState = {
   -1, -1, -1, -1, -1, -1, 1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1,
};
static final long[] jjtoToken = {
   0x7ffc201L,
};
static final long[] jjtoSkip = {
   0xbeL,
};
static final long[] jjtoSpecial = {
   0x80L,
};
static final long[] jjtoMore = {
   0x140L,
};
protected SimpleCharStream input_stream;
private final int[] jjrounds = new int[12];
private final int[] jjstateSet = new int[24];
StringBuffer image;
int jjimageLen;
int lengthOfMatch;
protected char curChar;
public ProbeParserTokenManager(SimpleCharStream stream)
{
   if (SimpleCharStream.staticFlag)
      throw new Error("ERROR: Cannot use a static CharStream class with a non-static lexical analyzer.");
   input_stream = stream;
}
public ProbeParserTokenManager(SimpleCharStream stream, int lexState)
{
   this(stream);
   SwitchTo(lexState);
}
public void ReInit(SimpleCharStream stream)
{
   jjmatchedPos = jjnewStateCnt = 0;
   curLexState = defaultLexState;
   input_stream = stream;
   ReInitRounds();
}
private void ReInitRounds()
{
   int i;
   jjround = 0x80000001;
   for (i = 12; i-- > 0;)
      jjrounds[i] = 0x80000000;
}
public void ReInit(SimpleCharStream stream, int lexState)
{
   ReInit(stream);
   SwitchTo(lexState);
}
public void SwitchTo(int lexState)
{
   if (lexState >= 2 || lexState < 0)
      throw new TokenMgrError("Error: Ignoring invalid lexical state : " + lexState + ". LegacyState unchanged.", TokenMgrError.INVALID_LEXICAL_STATE);
   else
      curLexState = lexState;
}

protected Token jjFillToken()
{
   Token t = Token.newToken(jjmatchedKind);
   t.kind = jjmatchedKind;
   String im = jjstrLiteralImages[jjmatchedKind];
   t.image = (im == null) ? input_stream.GetImage() : im;
   t.beginLine = input_stream.getBeginLine();
   t.beginColumn = input_stream.getBeginColumn();
   t.endLine = input_stream.getEndLine();
   t.endColumn = input_stream.getEndColumn();
   return t;
}

int curLexState = 0;
int defaultLexState = 0;
int jjnewStateCnt;
int jjround;
int jjmatchedPos;
int jjmatchedKind;

public Token getNextToken()
{
  Token specialToken = null;
  Token matchedToken;
  int curPos = 0;

  EOFLoop :
  while (true) {
      try {
          curChar = input_stream.BeginToken();
      } catch (IOException e) {
          jjmatchedKind = 0;
          matchedToken = jjFillToken();
          matchedToken.specialToken = specialToken;
          return matchedToken;
      }
      image = null;
      jjimageLen = 0;

      while (true) {
          switch (curLexState) {
              case 0:
                  try {
                      input_stream.backup(0);
                      while (curChar <= 32 && (0x100003600L & (1L << curChar)) != 0L)
                          curChar = input_stream.BeginToken();
                  } catch (IOException e1) {
                      continue EOFLoop;
                  }
                  jjmatchedKind = 0x7fffffff;
                  jjmatchedPos = 0;
                  curPos = jjMoveStringLiteralDfa0_0();
                  break;
              case 1:
                  jjmatchedKind = 0x7fffffff;
                  jjmatchedPos = 0;
                  curPos = jjMoveStringLiteralDfa0_1();
                  if (jjmatchedPos == 0 && jjmatchedKind > 8) {
                      jjmatchedKind = 8;
                  }
                  break;
          }
          if (jjmatchedKind != 0x7fffffff) {
              if (jjmatchedPos + 1 < curPos) input_stream.backup(curPos - jjmatchedPos - 1);
              if ((jjtoToken[jjmatchedKind >> 6] & (1L << (jjmatchedKind & 077))) != 0L) {
                  matchedToken = jjFillToken();
                  matchedToken.specialToken = specialToken;
                  if (jjnewLexState[jjmatchedKind] != -1) curLexState = jjnewLexState[jjmatchedKind];
                  return matchedToken;
              } else if ((jjtoSkip[jjmatchedKind >> 6] & (1L << (jjmatchedKind & 077))) != 0L) {
                  if ((jjtoSpecial[jjmatchedKind >> 6] & (1L << (jjmatchedKind & 077))) != 0L) {
                      matchedToken = jjFillToken();
                      if (specialToken == null) specialToken = matchedToken;
                      else {
                          matchedToken.specialToken = specialToken;
                          specialToken = (specialToken.next = matchedToken);
                      }
                      SkipLexicalActions(matchedToken);
                  } else SkipLexicalActions(null);
                  if (jjnewLexState[jjmatchedKind] != -1) curLexState = jjnewLexState[jjmatchedKind];
                  continue EOFLoop;
              }
              jjimageLen += jjmatchedPos + 1;
              if (jjnewLexState[jjmatchedKind] != -1) curLexState = jjnewLexState[jjmatchedKind];
              curPos = 0;
              jjmatchedKind = 0x7fffffff;
              try {
                  curChar = input_stream.readChar();
                  continue;
              } catch (IOException e1) {
              }
          }
          int error_line = input_stream.getEndLine();
          int error_column = input_stream.getEndColumn();
          String error_after = null;
          boolean EOFSeen = false;
          try {
              input_stream.readChar();
              input_stream.backup(1);
          } catch (IOException e1) {
              EOFSeen = true;
              error_after = curPos <= 1 ? "" : input_stream.GetImage();
              if (curChar == '\n' || curChar == '\r') {
                  error_line++;
                  error_column = 0;
              } else error_column++;
          }
          if (!EOFSeen) {
              input_stream.backup(1);
              error_after = curPos <= 1 ? "" : input_stream.GetImage();
          }
          throw new TokenMgrError(EOFSeen, curLexState, error_line, error_column, error_after, curChar, TokenMgrError.LEXICAL_ERROR);
      }
  }
}

void SkipLexicalActions(Token matchedToken)
{
   switch(jjmatchedKind)
   {
      default :
         break;
   }
}
}
