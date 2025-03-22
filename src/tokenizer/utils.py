
import unicodedata


def get_pair_freqs(ids: list, freqs = None):
    """computes frequencies for all consecutive pairs
    Inputs: a list of utf8 codes (0 - 255)
    Returns a dictionary with the frequencies of consecutive pairs
    example: ids = [12, 13, 18, 12, 13] -> {(12, 13): 2, (13, 18): 1, (18, 12): 1}"""
    freqs = {} if freqs is None else freqs
    for x, y in zip(ids, ids[1:]):
        freqs[(x, y)] = freqs.get((x, y), 0) + 1
    return freqs


def merge_pair(ids, pair, new_token):
    """merges the pair into a single token
    Inputs: a list of tokens, the pair to merge & the new_token
    Returns: updated ids with merged pair (all occurances)
    example: ids = [12, 13, 18, 12, 13], pair = (12, 13), new_token = 19 -> [19, 18, 19]"""
    updated_ids = []
    pos = 0
    while pos < len(ids):
        if pos < len(ids) - 1 and ids[pos] == pair[0] and ids[pos+1] == pair[1]:
            updated_ids.append(new_token)
            pos += 2 #skip the two merged tokens
        else:
            updated_ids.append(ids[pos])
            pos += 1
    return updated_ids


#helper functions that will be used in the saving process
def replace_control_characters(s: str) -> str:
    # we don't want to print control characters
    # which distort the output (e.g. \n or much worse)
    # https://stackoverflow.com/questions/4324790/removing-control-characters-from-a-string-in-python/19016117#19016117
    # http://www.unicode.org/reports/tr44/#GC_Values_Table
    chars = []
    for ch in s:
        if unicodedata.category(ch)[0] != "C":
            chars.append(ch) # this character is ok
        else:
            chars.append(f"\\u{ord(ch):04x}") # escape
    return "".join(chars)
    
def render_token(t: bytes) -> str:
    # pretty print a token, escaping control characters
    s = t.decode('utf-8', errors='replace')
    s = replace_control_characters(s)
    return s