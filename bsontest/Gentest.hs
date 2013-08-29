{-# LANGUAGE OverloadedStrings #-}

import qualified Text.XML.Light.Types as X
import Text.XML.Light.Input
import qualified Data.Bson as B
import Data.Bson (Field(..))
import Data.Bson.Binary
import Data.Binary.Put
import Data.Maybe
import Data.List
import Data.Text (Text, pack, unpack)
import Control.Monad
import System.IO
import qualified Data.ByteString.Lazy as BS

toBSON :: X.Content -> B.Field
toBSON e@(X.Elem (X.Element n a c _)) = toBSON' (X.qName n) (pack $ v "name") (v "value") c
    where v s = fromMaybe "" $ liftM X.attrVal $ find ((s ==) . X.qName . X.attrKey) a
toBSON e = error $ "expected XML element, got: " ++ show e

toBSON' :: String -> Text -> String -> [X.Content] -> B.Field
toBSON' "int32" n v _ = n := (B.Int32 $ read v)
toBSON' "bool" n v _ = n := (B.Bool $ read v)
toBSON' "double" n v _ = n := (B.Float $ read v)
toBSON' "str" n v _ = n := (B.String $ pack v)
toBSON' "doc" n _ xs = n := (B.Doc $ map toBSON $ maybeToList $ find isElem xs)
    where isElem (X.Elem _) = True
          isElem _ = False
toBSON' e n v c = error $ "unexpected XML element; elem = `" ++ e ++ "`; name = `" ++ (unpack n) ++ "`; value = `" ++ v ++ "`; contents = `" ++ show c ++ "`"

main' = mapM handleFile
    where handleFile name = readFile (name ++ ".xml") >>= (BS.writeFile (name ++ ".bson") . runPut . putDocument . (\((:=) _ (B.Doc fs)) -> fs) . toBSON . head . parseXML)
