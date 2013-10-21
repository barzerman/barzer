{-# LANGUAGE OverloadedStrings #-}

import Control.Arrow
import qualified Data.Map.Strict as M
import qualified Data.Text as T
import Data.List
import NLP.Snowball

gramCount = 3

genGrams c l | length l < c = []
             | otherwise = take c l : genGrams c (tail l)
genGramsU c l = concatMap (`genGrams` l) [1..c]

fixText = stems English . map (T.toLower . T.pack) . words . map (\c -> if c `elem` "?”“" then ' ' else c) . filter (`notElem` "-'")
parse2map = M.fromListWith (++) . map (second (fixText . drop 1) . break (== '|')) . lines
grammize = M.map $ genGramsU gramCount

invert = M.foldrWithKey (\d a m -> foldr (foldrer d) m a) M.empty
foldrer :: String -> [T.Text] -> M.Map [T.Text] [String] -> M.Map [T.Text] [String]
foldrer d i = M.insertWith (++) i [d]

toStats = M.map length

--reduceN c m = M.filterWithKey (\k v -> 

process cs = undefined

main' f = do
    c <- readFile f
    let d = process c
    return ()
