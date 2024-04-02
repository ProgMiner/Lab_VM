package ru.byprogminer.lamagraalvm.parser;

import org.antlr.v4.runtime.BaseErrorListener;
import org.antlr.v4.runtime.RecognitionException;
import org.antlr.v4.runtime.Recognizer;
import org.antlr.v4.runtime.misc.ParseCancellationException;

public final class ThrowingErrorListener extends BaseErrorListener {

    public static final ThrowingErrorListener INSTANCE = new ThrowingErrorListener();

    private ThrowingErrorListener() {}

    @Override
    public void syntaxError(
            Recognizer<?, ?> recognizer,
            Object offendingSymbol,
            int line,
            int charPositionInLine,
            String msg,
            RecognitionException e
    ) {
        throw makeException(line, charPositionInLine, msg);
    }

    public static ParseCancellationException makeException(int line, int charPositionInLine, String msg) {
        return new ParseCancellationException(line + ":" + charPositionInLine + " - " + msg);
    }
}
