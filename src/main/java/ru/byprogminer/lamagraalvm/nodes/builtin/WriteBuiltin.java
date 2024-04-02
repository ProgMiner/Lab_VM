package ru.byprogminer.lamagraalvm.nodes.builtin;

import com.oracle.truffle.api.dsl.GenerateNodeFactory;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.nodes.NodeInfo;
import lombok.Setter;

import java.io.PrintWriter;

@Setter
@GenerateNodeFactory
@NodeInfo(shortName = "write", description = "write() builtin")
public abstract class WriteBuiltin extends Builtin {

    private PrintWriter out;

    @Specialization
    public long write(long value) {
        out.println(value);
        out.flush();
        return 0;
    }
}
