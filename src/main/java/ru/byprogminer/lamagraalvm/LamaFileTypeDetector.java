package ru.byprogminer.lamagraalvm;

import com.oracle.truffle.api.TruffleFile;
import lombok.NoArgsConstructor;

import java.nio.charset.Charset;

@NoArgsConstructor
public class LamaFileTypeDetector implements TruffleFile.FileTypeDetector {

    public static final String FILE_EXTENSION = ".lama";

    @Override
    public String findMimeType(TruffleFile file) {
        final String name = file.getName();

        if (name != null && name.endsWith(FILE_EXTENSION)) {
            return LamaLanguage.MIME_TYPE;
        }

        return null;
    }

    @Override
    public Charset findEncoding(TruffleFile file) {
        return null;
    }
}
