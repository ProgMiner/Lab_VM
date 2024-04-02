package ru.byprogminer.lamagraalvm.runtime;

import com.oracle.truffle.api.frame.VirtualFrame;

public interface LamaRef {

    void assignLong(VirtualFrame frame, long value);

    void assignObject(VirtualFrame frame, Object value);
}
