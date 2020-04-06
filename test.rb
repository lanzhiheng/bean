array = []

Dir.glob('tests/*').each do |filename|
  result = `./bean #{filename}`

  next if (filename.end_with?("http.bn"))

  status = ($?).exitstatus
  if (status != 0)
    print "\u001b[31mF\u001b[0m in #{filename}"
    array.push(result)
  else
    print "\u001b[32m.\u001b[0m"
  end
end

if (!array.empty?)
  print "\n======================\n"

  array.each do |item|
    print item
  end
end
