import sys
import re

def remove_comments(lines : list[str]):
    """Remove comments "//" and "/* */" from lines."""
    in_block = False

    for line in lines:
        i = 0
        clean_line = ""

        while i < len(line):
            if not in_block:
                match_line_cmt = line.find('//', i)
                match_block_cmt = line.find('/*', i)

                if match_block_cmt > -1 and (match_block_cmt < match_line_cmt or match_line_cmt == -1):
                    clean_line += line[i:match_block_cmt]
                    i = match_block_cmt + 2
                    in_block = True
                elif match_line_cmt > -1:
                    clean_line += line[i:match_line_cmt]
                    i = len(line)
                else:
                    clean_line += line[i:len(line)]
                    i = len(line)
            else: # within a block
                match_block_cmt = line.find('*/', i)
                if match_block_cmt > -1:
                    i = match_block_cmt + 2
                    in_block = False
                else:
                    i = len(line)

        yield clean_line.strip()


def get_conditional_lines(source_path):
    """Return set of line numbers (1-indexed) that contain conditionals."""

    # Keywords if, else, for, while, switch. Ternary operator ?. Multiline statements with &&, ||
    # Multiline statements could also be captured more accurately by looking at the enclosing brackets
    CONDITIONAL_PATTERN = re.compile(r'\b(if|else|for|while|switch)\b|\?|&&|\|\|')

    with open(source_path, "r", encoding="utf-8") as source:
        return {i + 1 for i, line in enumerate(remove_comments(source)) if CONDITIONAL_PATTERN.search(line)}

def filter_non_conditional_lines_from_tracefile(tracefile_path, out_path):
    """Remove branch data from lines which do not contain a conditional in the source file."""
    with open(tracefile_path, "r", encoding="utf-8") as infile, open(out_path, "w", encoding="utf-8") as outfile:
        current_file = None
        conditional_lines = set()

        for line in infile:
            if line.startswith("SF:"):
                current_file = line.strip()[3:]
                conditional_lines = get_conditional_lines(current_file)
                outfile.write(line)
            elif line.startswith('BRDA:'):
                m = re.match(r"BRDA:(\d+)", line)
                if m and int(m.group(1)) in conditional_lines:
                    outfile.write(line)
            else:
                outfile.write(line) 

if __name__ == "__main__":
    filter_non_conditional_lines_from_tracefile(sys.argv[1], sys.argv[2])
