import pandas as pd
import numpy as np

# ==========================================
# 1. إعدادات الملفات
# ==========================================
HEALTHY_FILE = 'healthy.txt'
UNHEALTHY_FILE = 'unhealthy.txt'
TARGET_SAMPLES_PER_CLASS = 1000
FINAL_DATASET_NAME = 'MotorMind_Final_Dataset.csv'

def clean_data(df, label):
    """دالة لتنظيف البيانات والتركيز على العطل الصفري"""
    
    # تحويل البيانات لأرقام وتجاهل العناوين المتكررة
    df = df.apply(pd.to_numeric, errors='coerce')
    df = df.dropna()
    
    # تنظيف عام
    df = df[df['Temp'] > 0]
    df = df[df['DominantFreq'] >= 0]
    df = df[df['RMS'] >= 1.0]
    
    if label == 0:
        # الحالة السليمة: استبعاد القفزات الخرافية
        df = df[df['RMS'] <= 20.0]
        
    elif label == 1:
        # === التعديل الجديد ===
        # تجاهل كل بيانات العطل، والاحتفاظ "فقط" بالسطور اللي التيار فيها بصفر
        df = df[df['CurrentAmp'] == 0.0]
        
        # لو حابب تركز على القيم اللي شبه السطر بتاعك بالظبط في كل الحساسات، 
        # ممكن نضيق الفلتر أكتر، بس الفلتر ده (التيار = 0) كافي جداً لاصطياد العطل ده.
        
    return df

def augment_data(df, target_count):
    """دالة زيادة البيانات مع الحفاظ على بصمة العطل"""
    current_count = len(df)
    
    if current_count == 0:
        raise ValueError("الملف مفيهوش أي داتا مطابقة للشروط! اتأكد إن ملف unhealthy فيه سطور التيار بتاعها 0.00")
        
    if current_count >= target_count:
        return df.sample(n=target_count, random_state=42)
    
    needed = target_count - current_count
    print(f"Augmenting {needed} samples from {current_count} base samples to reach {target_count}...")
    
    sampled_df = df.sample(n=needed, replace=True, random_state=42).copy()
    
    features = ['RMS', 'Kurtosis', 'CrestFactor', 'P2P', 'CurrentAmp', 'DominantFreq', 'SpectralEnergy', 'Temp']
    for feature in features:
        # === تعديل مهم للعطل الصفري ===
        # لو الخاصية دي هي التيار (اللي إحنا قاصدين نخليه 0)، مش هنضيف عليه أي ضوضاء عشان يفضل 0.00 زي ما هو
        if feature == 'CurrentAmp' and df[feature].mean() == 0:
            continue
            
        std_dev = df[feature].std()
        # لو الانحراف المعياري بصفر (يعني كل القيم متطابقة)، هنحط ضوضاء ثابتة وصغيرة جداً
        if std_dev == 0:
            noise = np.random.normal(0, 0.01, size=needed)
        else:
            noise = np.random.normal(0, std_dev * 0.02, size=needed)
            
        sampled_df[feature] += noise
        
        if feature in ['RMS', 'CrestFactor', 'P2P', 'CurrentAmp', 'SpectralEnergy']:
            sampled_df[feature] = sampled_df[feature].clip(lower=0)

    augmented_df = pd.concat([df, sampled_df], ignore_index=True)
    return augmented_df

# ==========================================
# 2. تنفيذ الـ Pipeline
# ==========================================
print("🚀 Starting Data Pipeline...\n")

try:
    print(">> Processing Healthy Data (Label 0)...")
    healthy_df = pd.read_csv(HEALTHY_FILE)
    healthy_df = clean_data(healthy_df, label=0)
    healthy_1000 = augment_data(healthy_df, TARGET_SAMPLES_PER_CLASS)
    healthy_1000.to_csv('healthy_cleaned_1000.csv', index=False)
    print(f"✅ Healthy Data Ready: {len(healthy_1000)} rows.\n")

    print(">> Processing Unhealthy Data (Label 1)...")
    unhealthy_df = pd.read_csv(UNHEALTHY_FILE)
    unhealthy_df = clean_data(unhealthy_df, label=1)
    unhealthy_1000 = augment_data(unhealthy_df, TARGET_SAMPLES_PER_CLASS)
    unhealthy_1000.to_csv('unhealthy_cleaned_1000.csv', index=False)
    print(f"✅ Unhealthy Data Ready: {len(unhealthy_1000)} rows.\n")

    print(">> Merging into Final Dataset...")
    final_dataset = pd.concat([healthy_1000, unhealthy_1000], ignore_index=True)
    
    final_dataset = final_dataset.sample(frac=1, random_state=42).reset_index(drop=True)
    final_dataset.to_csv(FINAL_DATASET_NAME, index=False)
    print(f"🎉 SUCCESS! Final Dataset '{FINAL_DATASET_NAME}' generated with {len(final_dataset)} rows.")

except Exception as e:
    print(f"❌ Error occurred: {str(e)}")
