cmake_minimum_required(VERSION 3.10)
project(list)
 
set(myList a b c e d f )
list(LENGTH myList len)
# LENGTH는 리스트 item의 개수 반환
# GET은 해당 index의 리스트 item을 반환
message("Hello wolrd")
message("length = ${len}")

list(GET myList 2 -1 -2 -3 -4 letters)
message("letter = ${letters}")

list(APPEND myList k c u f)
list(GET myList 2 -1 -2 -3 -4 letters)
message("letter = ${letters}")

list(FIND myList u index)
message("index = ${index}")