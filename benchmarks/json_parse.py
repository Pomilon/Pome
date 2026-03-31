import time
import sys

class Parser:
    def __init__(self, source):
        self.source = source
        self.pos = 0
    def peek(self):
        return self.source[self.pos] if self.pos < len(self.source) else None
    def advance(self):
        c = self.peek()
        self.pos += 1
        return c
    def skipWhitespace(self):
        while self.peek() in (" ", "\n", "\t", "\r"):
            self.advance()
    def parse(self):
        self.skipWhitespace()
        c = self.peek()
        if c == "{": return self.parseObject()
        if c == "[": return self.parseArray()
        if c == "\"": return self.parseString()
        return self.parseLiteral()
    def parseObject(self):
        self.advance()
        obj = {}
        while True:
            self.skipWhitespace()
            c = self.peek()
            if c == "}" or c is None:
                if c == "}": self.advance()
                break
            key = self.parseString()
            self.skipWhitespace()
            self.advance()
            val = self.parse()
            obj[key] = val
            self.skipWhitespace()
            if self.peek() == ",": self.advance()
        return obj
    def parseArray(self):
        self.advance()
        arr = []
        while True:
            self.skipWhitespace()
            c = self.peek()
            if c == "]" or c is None:
                if c == "]": self.advance()
                break
            arr.append(self.parse())
            self.skipWhitespace()
            if self.peek() == ",": self.advance()
        return arr
    def parseString(self):
        self.advance()
        res = ""
        while True:
            c = self.advance()
            if c is None or c == "\"": break
            res += c
        return res
    def parseLiteral(self):
        res = ""
        while True:
            c = self.peek()
            if c is None or c in (" ", "\n", ",", "}", "]", ":"): break
            res += self.advance()
        return res

if __name__ == "__main__":
    if 'N' not in globals():
        N = 1000
    jsonStr = "{\"name\":\"Pome\",\"version\":\"1.0\",\"tags\":[\"fast\",\"dynamic\",\"fun\"],\"meta\":{\"author\":\"dev\"}}"
    start = time.time()
    last_obj = None
    for _ in range(N):
        p = Parser(jsonStr)
        last_obj = p.parse()
    end = time.time()
    print(last_obj['name'])
    print(f"Time: {end - start}s")
