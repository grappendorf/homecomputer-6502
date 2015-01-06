#!/bin/env ruby
# encoding: UTF-8

require 'serialport'

serial = SerialPort.open('/dev/ttyUSB0', 9600)

trap 'SIGINT' do
  serial.close
end

def cmd_save serial, filename
  File.open(filename, 'w') do |file|
    while line = serial.gets.chomp
      break if line =~ /\*EOF/
      file.puts line
      print '.'
    end
  end
  puts "\nSaved program to file #{filename}"
end

def cmd_load serial, filename
  begin
    File.open(filename).each do |line|
      serial.gets
      serial.puts line
    end
    serial.gets
    serial.puts '*EOF'
    puts "Loaded program from file #{filename}"
  rescue Errno::ENOENT => x
    serial.gets
    puts "File not found: #{filename}"
    serial.puts '!NOTFOUND'
  end
end

def cmd_dir serial
  Dir.new('programs').select{|f|f =~ /.+\..+/}.each do |filename|
    serial.gets
    serial.puts filename
  end
  serial.gets
  serial.puts '*EOF'
end

while true do
  begin
    line = serial.gets.chomp
    puts line.chars.select{|i| i.valid_encoding?}.join
    begin
      case line
        when /\*SAVE "((\w|\.| )+)"/
          cmd_save serial, "programs/#{$1}#{'.bas' unless $1.include? '.'}"
        when /\*LOAD "((\w|\.| )+)"/
          cmd_load serial, "programs/#{$1}#{'.bas' unless $1.include? '.'}"
        when /\*DIR/
          cmd_dir serial
      end
    rescue ArgumentError
    end
  rescue IOError => x
    break
  end
end
