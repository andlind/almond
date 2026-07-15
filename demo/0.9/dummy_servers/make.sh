#/bin/bash
for i in {10..99}; do
  cp webapp01.json "webapp${i}.json"
  sed -i "s/webapp01/webapp${i}/g" "webapp${i}.json"
done
#for i in {10..19}; do rm "webapp${i}.json"; done
