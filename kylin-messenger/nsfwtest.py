from nsfw_detector import predict
model = predict.load_model("/build-win/models/nsfw_mobilenet2.224x224.h5")
scores = predict.classify(model, "E:/Pictures/Pictures/92621277_p0.jpg")
print(scores)  # {"/abs/path/example.jpg": {"porn":..., "hentai":..., "sexy":..., "neutral":..., "drawings":...}}