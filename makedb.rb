#!/bin/ruby

while line = gets
  puts "DB \"#{line.chomp.gsub('"', "'")}\", 0x0d, 0x0a"
end
