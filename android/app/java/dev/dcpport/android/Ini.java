package dev.dcpport.android;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;

/** dcp.ini as the launcher writes it ("key = value" lines, # comments): reads a key, and sets one in place, keeping the
 *  other lines as they are. */
final class Ini {
    private static final String NEW_SECTION = "# Set from the Android app";
    private final File file;

    Ini(File file) {
        this.file = file;
    }

    String get(String key, String def) {
        for (String line : lines()) {
            String value = valueOf(line, key);
            if (value != null) {
                return value;
            }
        }
        return def;
    }

    boolean getBool(String key, boolean def) {
        String value = get(key, null);
        if (value == null) {
            return def;
        }
        value = value.toLowerCase();
        return value.equals("1") || value.equals("true") || value.equals("on") || value.equals("yes");
    }

    void set(String key, String value) {
        List<String> lines = lines();
        String entry = key + " = " + value;
        boolean replaced = false;
        for (int i = 0; i < lines.size(); ++i) {
            if (valueOf(lines.get(i), key) != null) {
                lines.set(i, entry);
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            if (!lines.contains(NEW_SECTION)) {
                if (!lines.isEmpty() && !lines.get(lines.size() - 1).isEmpty()) {
                    lines.add("");
                }
                lines.add(NEW_SECTION);
            }
            lines.add(lines.indexOf(NEW_SECTION) + 1, entry);
        }
        try {
            file.getParentFile().mkdirs();
            Files.write(file.toPath(), lines, StandardCharsets.UTF_8);
        } catch (IOException ignored) {
            // Read-only storage: the setting stays as it was, and the row shows it again next time.
        }
    }

    private List<String> lines() {
        try {
            return new ArrayList<>(Files.readAllLines(file.toPath(), StandardCharsets.UTF_8));
        } catch (IOException e) {
            return new ArrayList<>();
        }
    }

    /** The value when the line sets this key, else null. */
    private static String valueOf(String line, String key) {
        String trimmed = line.trim();
        if (trimmed.startsWith("#") || !trimmed.startsWith(key)) {
            return null;
        }
        String rest = trimmed.substring(key.length()).trim();
        return rest.startsWith("=") ? rest.substring(1).trim() : null;
    }
}
