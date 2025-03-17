


def get_pair_freqs(ids: list):
    """computes frequencies for all consecutive pairs
    Inputs: a list of utf8 codes (0 - 255)
    Returns a dictionary with the frequencies of consecutive pairs
    example: ids = [12, 13, 18, 12, 13] -> {(12, 13): 2, (13, 18): 1, (18, 12): 1}"""
    freqs = {}
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
        if ids[pos] == pair[0] and ids[pos+1] == pair[1] and pos < len(ids) - 1:
            updated_ids.append(new_token)
            pos += 2 #skip the two merged tokens
        else:
            updated_ids.append(ids[pos])
            pos += 1
    return updated_ids


def train(ids: list, vocab_size: int):
    vocab = list(range(256))
    new_token = 256
    merges = {} #to track merged ids
    while new_token < vocab_size:
        freqs = get_pair_freqs(ids)
        top_pair = max(freqs, key=freqs.get)
        if freqs[top_pair] == 1: break
        ids = merge_pair(ids, top_pair, new_token)
        vocab.append(new_token)
        merges[new_token] = chr(top_pair[0]), chr(top_pair[1])
        new_token += 1

    return vocab, merges

def main(txt: str, vocab_size: int):
    raw_bytes = txt.encode("utf-8")
    tokens = list(map(int, raw_bytes))
    vocab = train(tokens, vocab_size=vocab_size)
    return vocab



if __name__ == "__main__":
    # ids = [12, 13, 18, 12, 13]
    # freqs = get_pair_freqs(ids)
    # top_pair = max(freqs, key=freqs.get)
    # print(top_pair)
    
    # new_token = 19
    # print(merge_pair(ids, top_pair, new_token))

    file = open("test_text.txt", "r")
    txt = file.read()

    print(main(txt= "aaabdaaabac", vocab_size=270))

    