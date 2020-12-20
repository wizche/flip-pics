package main

import (
	"flag"
	"fmt"
	"image"
	"image/color"
	"image/jpeg"
	"io/ioutil"
	"log"
	"math"
	"os"
	"path"
	"path/filepath"
	"strings"

	"github.com/disintegration/imaging"
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

	if len(*input) <= 0 || len(*output) <= 0 {
		log.Fatal("Missing input/output parameters!")
	}

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
		baseName = strings.ReplaceAll(baseName, "_", "-")
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
		if img.Bounds().Max.Y > img.Bounds().Max.X {
			log.Printf("Image wider then higher, rotating 90 degree!")
			img = imaging.Rotate(img, 90.0, color.Gray{})
		}

		newSize := CalculateDimensions(img.Bounds().Max, 960.0, 540.0)

		m = imaging.Resize(img, newSize.X, newSize.Y, imaging.Lanczos)
		width := newSize.X
		height := newSize.Y

		log.Printf("Resized from %dx%d to %dx%d\n", img.Bounds().Max.X, img.Bounds().Max.Y, width, height)

		offsetX := int((960 - width) / 2.0)
		offsetY := int((540 - height) / 2.0)

		filename := fmt.Sprintf("%s_%dx%d_%dx%d_resized.jpg", baseName, width, height, offsetX, offsetY)
		out, err := os.Create(path.Join(outdir, filename))
		if err != nil {
			log.Fatal(err)
		}

		jpeg.Encode(out, m, nil)
		log.Printf("Created new image %s", filename)

		// write new image to file
		out.Close()
	}
}
