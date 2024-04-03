package ru.byprogminer.lamagraalvm.runtime;

import com.oracle.truffle.api.exception.AbstractTruffleException;
import com.oracle.truffle.api.nodes.Node;

public class LamaException extends AbstractTruffleException {

    public LamaException(Throwable cause, Node location) {
        super(cause.getMessage(), cause, UNLIMITED_STACK_TRACE, location);
    }
}
