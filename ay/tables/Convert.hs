module Main where

import System.Environment
import Text.XML.HXT.Parser.HtmlParsec
import Text.XML.HXT.XPath.XPathEval
import Text.XML.HXT.DOM.FormatXmlTree
import Data.Tree.NTree.TypeDefs
import Text.XML.HXT.DOM.TypeDefs
import System.IO
import Data.List
import Numeric

hdr = "uint32_t table[][2] =\n{\n\t"
fmt = intercalate ",\n\t" . map (\(a, b) -> "{0x" ++ showHex a ", 0x" ++ showHex b "}") 
end = "\n};\n"

convert xml = hdr ++ fmt (zip (col 1) (col 2)) ++ end
    where col n = map (fst . head . (readHex :: ReadS Int) . uw) (map (head . getXPath ("//td[" ++ show n ++ "]/text()")) $ tail $ getXPath ("//tr[@class=\"row\"]") xml)
          uw (NTree (XText t) _) = dropWhile (== '0') t
          uw s@_ = "-1"

main = do
    args <- getArgs
    fs <- readFile $ args !! 0
    putStrLn $ convert $ head $ parseHtmlContent fs
