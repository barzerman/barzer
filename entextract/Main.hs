{-# LANGUAGE NoMonomorphismRestriction #-}

import System.Environment
import Control.Arrow
import Control.Monad
import qualified Data.Map.Strict as M
import qualified Data.Text as T
import Data.List
import Data.Ord
import NLP.Snowball

gramCount = 3
maxLength = 2
markers = map T.pack ["a", "the"]
endShit = map T.pack ["a", "the", "in", "on", "to", "from", "of", "for", "until", "and", "or", "but", "of", "if", "i", "we", "us", "you"]
bothShit = map T.pack ["and", "or", "but", "of", "if", "i", "we", "us", "you"]
popThreshold = 2

data LinkedText = LinkedText {
        stemmed :: T.Text,
        origText :: T.Text
    } deriving (Show)
instance Ord LinkedText where
    compare = comparing stemmed
instance Eq LinkedText where
    x == y = stemmed x == stemmed y

mkLinked t = LinkedText (stem English t) t

type RevMap = M.Map [LinkedText] Int

genGrams c l | length l < c = []
             | otherwise = take c l : genGrams c (tail l)
genGramsU c l = concatMap (`genGrams` l) [1..c]

fixText = map (mkLinked . T.toLower . T.pack) . words . map (\c -> if c `elem` "?”“" then ' ' else c) . filter (`notElem` "-'")
parse2map = M.fromListWith (++) . map (second (fixText . drop 1) . break (== '|')) . lines
grammize = M.map $ genGramsU gramCount

invert = M.foldrWithKey (\d a m -> foldr (foldrer d) m a) M.empty
    where foldrer d i = M.insertWith (++) i [d]

toStats = M.map length

-- Remove sub-ngrams if they have the same statistical count.
reduce m = foldr foldrer m $ M.keys m
    where foldrer k m' = let l = M.lookup k m' in foldr (M.alter (f' l)) m' $ genGramsU (length k) k \\ [k]
          f' (Just v) jc@(Just c) | v == c = Nothing
                                  | otherwise = jc
          f' _ _ = Nothing

filterShit = M.filterWithKey (\g c -> stemmed (head g) `notElem` bothShit && stemmed (last g) `notElem` endShit)

filterPop = M.filter (< popThreshold)

filterLength = M.filterWithKey (\g _ -> maxLength >= length (filter ((`notElem` markers) . stemmed) g))

articlesHeur = M.filterWithKey (\g c -> c <= popThreshold && stemmed (head g) `elem` markers)

process :: String -> RevMap
process = prepare

prepare :: String -> RevMap 
prepare = filterLength . filterShit . filterPop . reduce . toStats . invert . grammize . parse2map

format :: RevMap -> String
format = unlines . map T.unpack . map (T.intercalate $ T.singleton ' ') . map (map (\t' -> let t = origText t' in if t `elem` markers then T.pack "opt(" `T.append` t `T.append` T.singleton ')' else t)) . M.keys

main' f = readFile f >>= (putStrLn . format . process)

main = do
    pname <- getProgName
    args <- getArgs
    if null args
        then print $ "Usage: " ++ pname ++ " datafile.txt"
        else main' $ head args
