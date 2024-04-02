package ru.byprogminer.lamagraalvm.nodes.builtin;

import com.oracle.truffle.api.dsl.GenerateNodeFactory;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.nodes.NodeInfo;
import lombok.Setter;
import lombok.SneakyThrows;

import java.io.BufferedReader;
import java.io.EOFException;
import java.io.PrintWriter;

@Setter
@GenerateNodeFactory
@NodeInfo(shortName = "read", description = "read() builtin")
public abstract class ReadBuiltin extends Builtin {

    private PrintWriter out;
    private BufferedReader in;

    @SneakyThrows
    @Specialization
    public long read() {
        out.print("> ");
        out.flush();

        final String line = in.readLine();

        if (line == null) {
            throw new EOFException();
        }

        return Long.parseLong(line);
    }
}
