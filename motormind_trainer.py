import pandas as pd
import numpy as np
import time
import m2cgen as m2c
import xgboost as xgb
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score

# === التعديل: اسم ملف الداتا سيت الجديد اللي طالع من سكريبت التجميع ===
DATASET_PATH = 'MotorMind_Final_Dataset.csv'

# 1. قراءة البيانات 
print("Loading MotorMind dataset...")
try:
    df = pd.read_csv(DATASET_PATH)
except FileNotFoundError:
    print(f"❌ Error: {DATASET_PATH} not found in this directory!")
    exit()

X = df.drop('Label', axis=1).values
y = df['Label'].values

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42, stratify=y)

# 2. تعريف الموديلات
models = {
    "Logistic Regression (Baseline)": LogisticRegression(max_iter=1000, random_state=42),
    "Random Forest (15 Trees)": RandomForestClassifier(n_estimators=15, max_depth=8, random_state=42),
    "XGBoost (Tiny Embedded)": xgb.XGBClassifier(n_estimators=15, max_depth=4, learning_rate=0.1, random_state=42)
}

results = []
best_f1 = 0
best_acc = 0
best_model_name = ""
best_model = None

print("\n⚙️ Training and Evaluating Models...\n")

# 3. حلقة المقارنة والتقييم
for name, model in models.items():
    model.fit(X_train, y_train)
    
    start_time = time.time()
    y_pred = model.predict(X_test)
    end_time = time.time()
    
    # زمن التنفيذ الاسترشادي على الكمبيوتر
    inf_time_us = ((end_time - start_time) / len(X_test)) * 1e6
    
    acc = accuracy_score(y_test, y_pred) * 100
    prec = precision_score(y_test, y_pred, zero_division=0) * 100
    rec = recall_score(y_test, y_pred, zero_division=0) * 100
    f1 = f1_score(y_test, y_pred, zero_division=0) * 100 
    
    # اختيار أفضل موديل (F1 ثم Accuracy)
    if f1 > best_f1 or (f1 == best_f1 and acc > best_acc):
        best_f1 = f1
        best_acc = acc
        best_model_name = name
        best_model = model
    
    try:
        c_code = m2c.export_to_c(model)
        code_size_kb = len(c_code.encode('utf-8')) / 1024
    except Exception as e:
        code_size_kb = float('nan') 
        
    results.append({
        "Model": name,
        "Accuracy (%)": f"{acc:.2f}",
        "Precision (%)": f"{prec:.2f}",
        "Recall (%)": f"{rec:.2f}",
        "F1-Score (%)": f"{f1:.2f}",
        "Est. Code (KB)": f"{code_size_kb:.2f}",
        "Time (us/sample)*": f"{inf_time_us:.2f}" # إضافة نجمة للتوضيح
    })

# 4. طباعة جدول النتائج النهائي
results_df = pd.DataFrame(results)
print("="*95)
print("🏆 MotorMind Edge AI Models Benchmark Results 🏆".center(95))
print("="*95)
print(results_df.to_string(index=False))
print("="*95)
print("* Time (us/sample) is PC-relative inference time, not actual ESP32 hardware time.\n")

# 5. التصدير التلقائي لأفضل موديل
print(f"🌟 Best Model Selected: {best_model_name} (F1-Score: {best_f1:.2f}%)")
print("Exporting Best Model to C++...")

try:
    final_c_code = m2c.export_to_c(best_model)
    with open("MotorModel.h", "w") as f:
        f.write(final_c_code)
    print("✅ SUCCESS: 'MotorModel.h' has been generated in your folder!")
    
except Exception as e:
    print(f"❌ Error exporting the best model ({best_model_name}): {e}")
    print("⚠️ Fallback: Switching to Random Forest for guaranteed C++ export...")
    fallback_model = models["Random Forest (15 Trees)"]
    final_c_code = m2c.export_to_c(fallback_model)
    with open("MotorModel.h", "w") as f:
        f.write(final_c_code)
    print("✅ SUCCESS: 'MotorModel.h' (Random Forest) has been generated in your folder!")