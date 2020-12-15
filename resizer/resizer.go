package main

import (
	"flag"
	"fmt"
	"image"
	"image/jpeg"
	"io/ioutil"
	"log"
	"math"
	"os"
	"path"
	"path/filepath"
	"strings"

	"github.com/nfnt/resize"
)

// CalculateDimensions returns the most suitable image for the given size
func CalculateDimensions(currentSize image.Point, maxWidth float64, maxHeight float64) image.Point {
	var sourceWidth = currentSize.X
	var sourceHeight = currentSize.Y

	var widthPercent = maxWidth / float64(sourceWidth)
	var heightPercent = maxHeight / float64(sourceHeight)

	percent := widthPercent
	if heightPercent < widthPercent {
		percent = heightPercent
	}

	var destWidth = int(math.Floor(float64(sourceWidth) * percent))
	var destHeight = int(math.Floor(float64(sourceHeight) * percent))

	return image.Point{destWidth, destHeight}
}

func main() {
	input := flag.String("input", "", "input directory")
	output := flag.String("output", "", "output directory")
	flag.Parse()

	outdir := *output
	log.Printf("Processing image from %s, resizing into %s\n", *input, outdir)

	items, _ := ioutil.ReadDir(*input)
	for _, item := range items {
		if item.IsDir() {
			continue
		}
		if strings.HasSuffix("jpg", item.Name()) || strings.HasSuffix("jpeg", item.Name()) {
			log.Printf("%s is not a jpg image\n", item.Name())
			continue

		}
		baseName := strings.TrimSuffix(item.Name(), filepath.Ext(item.Name()))
		fullpath := path.Join(*input, item.Name())
		log.Printf("Processing %s", fullpath)
		file, err := os.Open(fullpath)
		if err != nil {
			log.Fatal(err)
		}
		img, err := jpeg.Decode(file)
		if err != nil {
			log.Fatal(err)
		}
		file.Close()

		var m image.Image
		newSize := CalculateDimensions(img.Bounds().Max, 960.0, 540.0)

		m = resize.Resize(uint(newSize.X), uint(newSize.Y), img, resize.Lanczos3)
		filename := fmt.Sprintf("%s_%dx%d_%dx%d_resized.jpg", baseName, newSize.X, newSize.Y, int((960-newSize.X)/2.0), int((540-newSize.Y)/2.0))
		log.Printf("Resized from %dx%d to %dx%d -> %s\n", img.Bounds().Max.X, img.Bounds().Max.Y, newSize.X, newSize.Y, filename)
		out, err := os.Create(path.Join(outdir, filename))
		if err != nil {
			log.Fatal(err)
		}
		// write new image to file
		jpeg.Encode(out, m, nil)
		out.Close()
	}
}
